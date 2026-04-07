"""
Fine-tuning LLaMA-3 8B on Hindi Riddle Dataset
Using QLoRA (4-bit quantization + LoRA adapters) for memory-efficient training.

Architecture:
  - Base: meta-llama/Meta-Llama-3-8B-Instruct
  - Quantization: BitsAndBytes 4-bit (NF4)
  - PEFT: LoRA (r=16, alpha=32) on q_proj, v_proj, k_proj, o_proj
  - Training: SFTTrainer (TRL) with causal language modeling
  - Logging: Weights & Biases (optional)
"""

import os
import sys
import json
import math
import logging
import argparse
from pathlib import Path
from datetime import datetime

import torch
from datasets import load_from_disk, DatasetDict
from transformers import (
    AutoTokenizer,
    AutoModelForCausalLM,
    BitsAndBytesConfig,
    TrainingArguments,
    EarlyStoppingCallback,
)
from peft import (
    LoraConfig,
    get_peft_model,
    prepare_model_for_kbit_training,
    TaskType,
)
from trl import SFTTrainer, DataCollatorForCompletionOnlyLM
import wandb
from dotenv import load_dotenv

load_dotenv()

# ─────────────────────────────────────────────────────────────────────────────
# Logging Setup
# ─────────────────────────────────────────────────────────────────────────────
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s | %(levelname)s | %(message)s",
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler("training.log"),
    ]
)
logger = logging.getLogger(__name__)


# ─────────────────────────────────────────────────────────────────────────────
# Default Configuration
# ─────────────────────────────────────────────────────────────────────────────
DEFAULT_CONFIG = {
    # Model
    "model_name": os.getenv("MODEL_NAME", "meta-llama/Meta-Llama-3-8B-Instruct"),
    "hf_token": os.getenv("HF_TOKEN", None),

    # Data
    "data_dir": "./data",
    "max_seq_length": 512,

    # LoRA
    "lora_r": 16,
    "lora_alpha": 32,
    "lora_dropout": 0.05,
    "lora_target_modules": [
        "q_proj", "k_proj", "v_proj", "o_proj",
        "gate_proj", "up_proj", "down_proj"
    ],

    # Quantization
    "use_4bit": True,
    "bnb_4bit_compute_dtype": "bfloat16",
    "bnb_4bit_quant_type": "nf4",
    "use_nested_quant": False,

    # Training
    "output_dir": "./outputs/llama3-hindi-riddle-solver",
    "num_train_epochs": 5,
    "per_device_train_batch_size": 2,
    "per_device_eval_batch_size": 2,
    "gradient_accumulation_steps": 4,
    "gradient_checkpointing": True,
    "learning_rate": 2e-4,
    "weight_decay": 0.001,
    "max_grad_norm": 0.3,
    "warmup_ratio": 0.03,
    "lr_scheduler_type": "cosine",
    "optim": "paged_adamw_32bit",
    "fp16": False,
    "bf16": True,
    "logging_steps": 10,
    "eval_strategy": "steps",
    "eval_steps": 25,
    "save_strategy": "steps",
    "save_steps": 50,
    "save_total_limit": 3,
    "load_best_model_at_end": True,
    "metric_for_best_model": "eval_loss",
    "greater_is_better": False,
    "report_to": "none",    # Set to "wandb" to enable W&B logging
    "run_name": f"llama3-hindi-riddles-{datetime.now().strftime('%Y%m%d-%H%M')}",

    # Packing
    "packing": False,

    # Early stopping
    "early_stopping_patience": 3,
}


# ─────────────────────────────────────────────────────────────────────────────
# Model Loading
# ─────────────────────────────────────────────────────────────────────────────
def load_model_and_tokenizer(config: dict):
    """Load quantized LLaMA 3 model and tokenizer."""
    model_name = config["model_name"]
    hf_token = config.get("hf_token")

    logger.info(f"Loading tokenizer from: {model_name}")
    tokenizer = AutoTokenizer.from_pretrained(
        model_name,
        token=hf_token,
        trust_remote_code=True,
    )

    # LLaMA 3 uses <|end_of_text|> as EOS; pad with that
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "right"   # Important for SFT with causal LM

    # BitsAndBytes 4-bit quantization config
    if config["use_4bit"]:
        compute_dtype = getattr(torch, config["bnb_4bit_compute_dtype"])
        bnb_config = BitsAndBytesConfig(
            load_in_4bit=True,
            bnb_4bit_quant_type=config["bnb_4bit_quant_type"],
            bnb_4bit_compute_dtype=compute_dtype,
            bnb_4bit_use_double_quant=config["use_nested_quant"],
        )
        logger.info("4-bit quantization enabled (QLoRA mode)")
    else:
        bnb_config = None
        logger.info("Loading model in full precision")

    logger.info(f"Loading model: {model_name}")
    model = AutoModelForCausalLM.from_pretrained(
        model_name,
        quantization_config=bnb_config,
        device_map="auto",
        token=hf_token,
        trust_remote_code=True,
        attn_implementation="eager",    # Use eager attention for compatibility
    )

    if config["use_4bit"]:
        model = prepare_model_for_kbit_training(
            model,
            use_gradient_checkpointing=config["gradient_checkpointing"]
        )

    if config["gradient_checkpointing"]:
        model.config.use_cache = False

    logger.info(f"Model loaded. Parameters: {sum(p.numel() for p in model.parameters()):,}")
    return model, tokenizer


# ─────────────────────────────────────────────────────────────────────────────
# LoRA Configuration
# ─────────────────────────────────────────────────────────────────────────────
def apply_lora(model, config: dict):
    """Apply LoRA adapters to the model."""
    lora_config = LoraConfig(
        r=config["lora_r"],
        lora_alpha=config["lora_alpha"],
        lora_dropout=config["lora_dropout"],
        target_modules=config["lora_target_modules"],
        bias="none",
        task_type=TaskType.CAUSAL_LM,
    )
    model = get_peft_model(model, lora_config)

    trainable, total = 0, 0
    for _, param in model.named_parameters():
        total += param.numel()
        if param.requires_grad:
            trainable += param.numel()
    logger.info(
        f"LoRA applied. Trainable: {trainable:,} / {total:,} "
        f"({100 * trainable / total:.2f}%)"
    )
    return model


# ─────────────────────────────────────────────────────────────────────────────
# Dataset Loading
# ─────────────────────────────────────────────────────────────────────────────
def load_dataset(data_dir: str) -> DatasetDict:
    """Load the prepared riddle dataset from disk."""
    logger.info(f"Loading dataset from: {data_dir}")
    dataset = load_from_disk(data_dir)
    logger.info(
        f"Dataset loaded — "
        f"Train: {len(dataset['train'])} | "
        f"Val: {len(dataset['validation'])} | "
        f"Test: {len(dataset['test'])}"
    )
    return dataset


# ─────────────────────────────────────────────────────────────────────────────
# Training Arguments
# ─────────────────────────────────────────────────────────────────────────────
def build_training_args(config: dict) -> TrainingArguments:
    """Build HuggingFace TrainingArguments from config."""
    return TrainingArguments(
        output_dir=config["output_dir"],
        num_train_epochs=config["num_train_epochs"],
        per_device_train_batch_size=config["per_device_train_batch_size"],
        per_device_eval_batch_size=config["per_device_eval_batch_size"],
        gradient_accumulation_steps=config["gradient_accumulation_steps"],
        gradient_checkpointing=config["gradient_checkpointing"],
        optim=config["optim"],
        learning_rate=config["learning_rate"],
        weight_decay=config["weight_decay"],
        max_grad_norm=config["max_grad_norm"],
        warmup_ratio=config["warmup_ratio"],
        lr_scheduler_type=config["lr_scheduler_type"],
        fp16=config["fp16"],
        bf16=config["bf16"],
        logging_steps=config["logging_steps"],
        evaluation_strategy=config["eval_strategy"],
        eval_steps=config["eval_steps"],
        save_strategy=config["save_strategy"],
        save_steps=config["save_steps"],
        save_total_limit=config["save_total_limit"],
        load_best_model_at_end=config["load_best_model_at_end"],
        metric_for_best_model=config["metric_for_best_model"],
        greater_is_better=config["greater_is_better"],
        report_to=config["report_to"],
        run_name=config["run_name"],
        group_by_length=True,
        dataloader_num_workers=2,
    )


# ─────────────────────────────────────────────────────────────────────────────
# Main Training Loop
# ─────────────────────────────────────────────────────────────────────────────
def train(config: dict):
    """Full fine-tuning pipeline."""

    # Initialize W&B if enabled
    if config.get("report_to") == "wandb":
        wandb_key = os.getenv("WANDB_API_KEY")
        if wandb_key:
            wandb.login(key=wandb_key)
        wandb.init(
            project=os.getenv("WANDB_PROJECT", "hindi-riddle-solver"),
            name=config["run_name"],
        )

    # Save config
    os.makedirs(config["output_dir"], exist_ok=True)
    config_path = os.path.join(config["output_dir"], "training_config.json")
    with open(config_path, "w") as f:
        json.dump(config, f, indent=2, default=str)
    logger.info(f"Config saved to: {config_path}")

    # Load model & tokenizer
    model, tokenizer = load_model_and_tokenizer(config)

    # Apply LoRA
    model = apply_lora(model, config)

    # Load dataset
    dataset = load_dataset(config["data_dir"])

    # Training arguments
    training_args = build_training_args(config)

    # Callbacks
    callbacks = []
    if config.get("early_stopping_patience"):
        callbacks.append(
            EarlyStoppingCallback(
                early_stopping_patience=config["early_stopping_patience"]
            )
        )

    # SFT Trainer
    trainer = SFTTrainer(
        model=model,
        tokenizer=tokenizer,
        train_dataset=dataset["train"],
        eval_dataset=dataset["validation"],
        dataset_text_field="text",
        max_seq_length=config["max_seq_length"],
        packing=config["packing"],
        args=training_args,
        callbacks=callbacks,
    )

    logger.info("=" * 60)
    logger.info("Starting training...")
    logger.info(f"  Model         : {config['model_name']}")
    logger.info(f"  Train samples : {len(dataset['train'])}")
    logger.info(f"  Val samples   : {len(dataset['validation'])}")
    logger.info(f"  Epochs        : {config['num_train_epochs']}")
    logger.info(f"  Batch size    : {config['per_device_train_batch_size']} × {config['gradient_accumulation_steps']} accum")
    logger.info(f"  LR            : {config['learning_rate']}")
    logger.info(f"  Output dir    : {config['output_dir']}")
    logger.info("=" * 60)

    # Train
    train_result = trainer.train()

    # Save the final adapter
    final_model_path = os.path.join(config["output_dir"], "final_adapter")
    trainer.model.save_pretrained(final_model_path)
    tokenizer.save_pretrained(final_model_path)
    logger.info(f"Final LoRA adapter saved to: {final_model_path}")

    # Save training metrics
    metrics = train_result.metrics
    trainer.log_metrics("train", metrics)
    trainer.save_metrics("train", metrics)

    logger.info("Training complete!")
    logger.info(f"Training loss: {metrics.get('train_loss', 'N/A'):.4f}")

    if config.get("report_to") == "wandb":
        wandb.finish()

    return trainer


# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────
def parse_args():
    parser = argparse.ArgumentParser(
        description="Fine-tune LLaMA-3 8B on Hindi riddles with QLoRA"
    )
    parser.add_argument("--model-name", type=str,
                        default=DEFAULT_CONFIG["model_name"])
    parser.add_argument("--data-dir", type=str,
                        default=DEFAULT_CONFIG["data_dir"])
    parser.add_argument("--output-dir", type=str,
                        default=DEFAULT_CONFIG["output_dir"])
    parser.add_argument("--epochs", type=int,
                        default=DEFAULT_CONFIG["num_train_epochs"])
    parser.add_argument("--batch-size", type=int,
                        default=DEFAULT_CONFIG["per_device_train_batch_size"])
    parser.add_argument("--lr", type=float,
                        default=DEFAULT_CONFIG["learning_rate"])
    parser.add_argument("--lora-r", type=int,
                        default=DEFAULT_CONFIG["lora_r"])
    parser.add_argument("--lora-alpha", type=int,
                        default=DEFAULT_CONFIG["lora_alpha"])
    parser.add_argument("--max-seq-length", type=int,
                        default=DEFAULT_CONFIG["max_seq_length"])
    parser.add_argument("--no-4bit", action="store_true",
                        help="Disable 4-bit quantization")
    parser.add_argument("--wandb", action="store_true",
                        help="Enable Weights & Biases logging")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    config = DEFAULT_CONFIG.copy()
    config.update({
        "model_name": args.model_name,
        "data_dir": args.data_dir,
        "output_dir": args.output_dir,
        "num_train_epochs": args.epochs,
        "per_device_train_batch_size": args.batch_size,
        "learning_rate": args.lr,
        "lora_r": args.lora_r,
        "lora_alpha": args.lora_alpha,
        "max_seq_length": args.max_seq_length,
        "use_4bit": not args.no_4bit,
        "report_to": "wandb" if args.wandb else "none",
    })
    train(config)
