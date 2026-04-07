"""
Evaluation Script for Hindi Riddle Solver
Computes:
  - Exact Match Accuracy
  - BERTScore (F1) using bert-base-multilingual-cased
  - ROUGE-L
  - BLEU score
  - Per-category and per-difficulty breakdown
  - Saves detailed results to JSON and generates plots
"""

import os
import json
import argparse
import logging
import sys
from pathlib import Path
from typing import List, Dict, Optional
from datetime import datetime

import torch
import numpy as np
import pandas as pd
from tqdm import tqdm
from datasets import load_from_disk

from transformers import AutoTokenizer, AutoModelForCausalLM, BitsAndBytesConfig
from peft import PeftModel

import evaluate
from bert_score import score as bert_score_fn
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use("Agg")   # Non-interactive backend
import seaborn as sns

logging.basicConfig(level=logging.INFO, format="%(asctime)s | %(levelname)s | %(message)s")
logger = logging.getLogger(__name__)

# ─────────────────────────────────────────────────────────────────────────────
# Model Inference
# ─────────────────────────────────────────────────────────────────────────────

def load_model_for_inference(
    base_model_name: str,
    adapter_path: Optional[str] = None,
    use_4bit: bool = True,
    hf_token: Optional[str] = None
):
    """Load the fine-tuned model (or base model) for inference."""
    tokenizer = AutoTokenizer.from_pretrained(
        base_model_name, token=hf_token, trust_remote_code=True
    )
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "left"    # Left-pad for generation

    if use_4bit:
        bnb_config = BitsAndBytesConfig(
            load_in_4bit=True,
            bnb_4bit_quant_type="nf4",
            bnb_4bit_compute_dtype=torch.bfloat16,
        )
    else:
        bnb_config = None

    model = AutoModelForCausalLM.from_pretrained(
        base_model_name,
        quantization_config=bnb_config,
        device_map="auto",
        token=hf_token,
        trust_remote_code=True,
    )

    if adapter_path and Path(adapter_path).exists():
        logger.info(f"Loading LoRA adapter from: {adapter_path}")
        model = PeftModel.from_pretrained(model, adapter_path)
        model = model.merge_and_unload()   # Merge LoRA weights for faster inference
        logger.info("LoRA weights merged successfully.")

    model.eval()
    return model, tokenizer


def generate_answer(
    model,
    tokenizer,
    riddle_text: str,
    max_new_tokens: int = 200,
    temperature: float = 0.1,
    do_sample: bool = False,
) -> str:
    """Generate an answer for a given riddle using the chat template."""
    system_msg = (
        "आप एक विशेषज्ञ हिंदी पहेली-समाधानकर्ता और सांस्कृतिक ज्ञान विशेषज्ञ हैं। "
        "आपको हिंदी में दी गई पहेलियों का सटीक और सार्थक उत्तर देना है। "
        "उत्तर हमेशा हिंदी में दें और एक छोटा सा स्पष्टीकरण भी जोड़ें।"
    )
    prompt = (
        "<|begin_of_text|>"
        "<|start_header_id|>system<|end_header_id|>\n\n"
        f"{system_msg}<|eot_id|>"
        "<|start_header_id|>user<|end_header_id|>\n\n"
        f"निम्नलिखित पहेली का उत्तर दीजिए:\n\n{riddle_text}<|eot_id|>"
        "<|start_header_id|>assistant<|end_header_id|>\n\n"
    )

    inputs = tokenizer(
        prompt, return_tensors="pt", truncation=True, max_length=512
    ).to(model.device)

    with torch.no_grad():
        outputs = model.generate(
            **inputs,
            max_new_tokens=max_new_tokens,
            temperature=temperature if do_sample else 1.0,
            do_sample=do_sample,
            pad_token_id=tokenizer.eos_token_id,
            eos_token_id=tokenizer.eos_token_id,
        )

    # Decode only the newly generated tokens
    generated = outputs[0][inputs["input_ids"].shape[1]:]
    return tokenizer.decode(generated, skip_special_tokens=True).strip()


# ─────────────────────────────────────────────────────────────────────────────
# Metric Computation
# ─────────────────────────────────────────────────────────────────────────────

def compute_exact_match(predictions: List[str], references: List[str]) -> float:
    """Case-insensitive exact match accuracy."""
    matches = sum(
        pred.strip().lower() == ref.strip().lower()
        for pred, ref in zip(predictions, references)
    )
    return matches / len(predictions) if predictions else 0.0


def extract_answer_from_output(output: str) -> str:
    """
    Extract the answer from the model's output.
    Looks for patterns like 'उत्तर: X' or '**उत्तर:** X'.
    """
    # Try to find explicit answer marker
    lines = output.split("\n")
    for line in lines:
        if "उत्तर" in line and ":" in line:
            # Extract text after colon, remove bold markers
            answer_part = line.split(":", 1)[-1]
            answer_part = answer_part.replace("**", "").strip()
            if answer_part:
                return answer_part
    # Fall back to first line or full output
    first_line = lines[0].replace("**", "").strip() if lines else output
    return first_line if first_line else output


def compute_bertscore(
    predictions: List[str],
    references: List[str],
    lang: str = "hi"
) -> Dict[str, float]:
    """Compute BERTScore using multilingual BERT."""
    logger.info("Computing BERTScore...")
    P, R, F1 = bert_score_fn(
        predictions, references,
        model_type="bert-base-multilingual-cased",
        lang=lang,
        verbose=False
    )
    return {
        "bertscore_precision": P.mean().item(),
        "bertscore_recall": R.mean().item(),
        "bertscore_f1": F1.mean().item(),
    }


def compute_rouge(
    predictions: List[str],
    references: List[str]
) -> Dict[str, float]:
    """Compute ROUGE-L score."""
    rouge = evaluate.load("rouge")
    results = rouge.compute(
        predictions=predictions,
        references=references,
        rouge_types=["rougeL"],
        use_aggregator=True
    )
    return {"rouge_l": results["rougeL"]}


def compute_all_metrics(
    predictions: List[str],
    references: List[str],
    raw_answers: List[str],     # Extracted answers vs full output
) -> Dict[str, float]:
    """Compute all evaluation metrics."""
    metrics = {}

    # Exact match on extracted answers
    metrics["exact_match_answer"] = compute_exact_match(raw_answers, references)

    # Exact match on full output (more lenient)
    metrics["exact_match_output"] = compute_exact_match(predictions, references)

    # BERTScore
    bs = compute_bertscore(raw_answers, references)
    metrics.update(bs)

    # ROUGE-L
    rouge = compute_rouge(raw_answers, references)
    metrics.update(rouge)

    return metrics


# ─────────────────────────────────────────────────────────────────────────────
# Plotting
# ─────────────────────────────────────────────────────────────────────────────

def plot_results(results_df: pd.DataFrame, output_dir: str):
    """Generate evaluation plots."""
    plots_dir = os.path.join(output_dir, "plots")
    os.makedirs(plots_dir, exist_ok=True)

    # Color palette
    palette = sns.color_palette("husl", 8)
    sns.set_theme(style="whitegrid", font="DejaVu Sans")

    # 1. Per-category accuracy
    if "category" in results_df.columns:
        cat_acc = results_df.groupby("category")["is_correct"].mean().sort_values()
        fig, ax = plt.subplots(figsize=(10, 5))
        bars = ax.barh(cat_acc.index, cat_acc.values * 100, color=palette)
        ax.set_xlabel("Accuracy (%)", fontsize=12)
        ax.set_title("Riddle Solving Accuracy by Category", fontsize=14, fontweight="bold")
        ax.bar_label(bars, fmt="%.1f%%", padding=3)
        plt.tight_layout()
        plt.savefig(os.path.join(plots_dir, "accuracy_by_category.png"), dpi=150)
        plt.close()
        logger.info("Saved: accuracy_by_category.png")

    # 2. Per-difficulty accuracy
    if "difficulty" in results_df.columns:
        diff_order = ["easy", "medium", "hard"]
        diff_acc = (
            results_df.groupby("difficulty")["is_correct"].mean()
            .reindex(diff_order, fill_value=0)
        )
        fig, ax = plt.subplots(figsize=(6, 4))
        bars = ax.bar(diff_acc.index, diff_acc.values * 100,
                      color=[palette[0], palette[3], palette[6]])
        ax.set_ylabel("Accuracy (%)", fontsize=12)
        ax.set_title("Accuracy by Difficulty Level", fontsize=14, fontweight="bold")
        ax.bar_label(bars, fmt="%.1f%%", padding=3)
        plt.tight_layout()
        plt.savefig(os.path.join(plots_dir, "accuracy_by_difficulty.png"), dpi=150)
        plt.close()
        logger.info("Saved: accuracy_by_difficulty.png")

    # 3. BERTScore distribution
    if "bertscore_f1" in results_df.columns:
        fig, ax = plt.subplots(figsize=(8, 4))
        ax.hist(results_df["bertscore_f1"], bins=20, color=palette[2], edgecolor="white")
        ax.axvline(results_df["bertscore_f1"].mean(), color="red",
                   linestyle="--", label=f"Mean: {results_df['bertscore_f1'].mean():.3f}")
        ax.set_xlabel("BERTScore F1", fontsize=12)
        ax.set_ylabel("Count", fontsize=12)
        ax.set_title("Distribution of BERTScore F1 Scores", fontsize=14, fontweight="bold")
        ax.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(plots_dir, "bertscore_distribution.png"), dpi=150)
        plt.close()
        logger.info("Saved: bertscore_distribution.png")

    logger.info(f"All plots saved to: {plots_dir}")


# ─────────────────────────────────────────────────────────────────────────────
# Main Evaluation Loop
# ─────────────────────────────────────────────────────────────────────────────

def evaluate_model(
    model,
    tokenizer,
    dataset_split,
    output_dir: str,
    max_new_tokens: int = 200,
    split_name: str = "test"
):
    """Run inference on a dataset split and compute all metrics."""
    os.makedirs(output_dir, exist_ok=True)

    predictions = []
    raw_answers = []
    references = []
    detailed_results = []

    logger.info(f"Running inference on {len(dataset_split)} samples...")

    for i, sample in enumerate(tqdm(dataset_split, desc=f"Evaluating [{split_name}]")):
        riddle = sample.get("riddle", "")
        answer = sample.get("answer", "")

        output = generate_answer(model, tokenizer, riddle, max_new_tokens=max_new_tokens)
        extracted = extract_answer_from_output(output)

        predictions.append(output)
        raw_answers.append(extracted)
        references.append(answer)

        is_correct = extracted.strip().lower() == answer.strip().lower()

        from bert_score import score as bs_fn
        _, _, f1 = bs_fn([extracted], [answer],
                         model_type="bert-base-multilingual-cased",
                         lang="hi", verbose=False)

        detailed_results.append({
            "index": i,
            "riddle": riddle,
            "ground_truth": answer,
            "predicted_answer": extracted,
            "full_output": output,
            "is_correct": is_correct,
            "bertscore_f1": f1.item(),
            "category": sample.get("category", "unknown"),
            "difficulty": sample.get("difficulty", "unknown"),
        })

    # Overall metrics
    metrics = compute_all_metrics(predictions, references, raw_answers)
    logger.info("\n" + "=" * 50)
    logger.info(f"EVALUATION RESULTS [{split_name}]")
    logger.info("=" * 50)
    for k, v in metrics.items():
        logger.info(f"  {k:35s}: {v:.4f}")
    logger.info("=" * 50)

    # Save results
    results_df = pd.DataFrame(detailed_results)
    results_csv = os.path.join(output_dir, f"{split_name}_predictions.csv")
    results_df.to_csv(results_csv, index=False, encoding="utf-8-sig")
    logger.info(f"Predictions saved to: {results_csv}")

    metrics_path = os.path.join(output_dir, f"{split_name}_metrics.json")
    with open(metrics_path, "w") as f:
        json.dump(metrics, f, indent=2)
    logger.info(f"Metrics saved to: {metrics_path}")

    # Plots
    plot_results(results_df, output_dir)

    return metrics, results_df


# ─────────────────────────────────────────────────────────────────────────────
# CLI
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Evaluate the LLaMA-3 Hindi Riddle Solver"
    )
    parser.add_argument("--base-model", type=str,
                        default="meta-llama/Meta-Llama-3-8B-Instruct")
    parser.add_argument("--adapter-path", type=str, default=None,
                        help="Path to fine-tuned LoRA adapter")
    parser.add_argument("--data-dir", type=str, default="./data")
    parser.add_argument("--output-dir", type=str, default="./eval_results")
    parser.add_argument("--split", type=str, default="test",
                        choices=["train", "validation", "test"])
    parser.add_argument("--max-new-tokens", type=int, default=200)
    parser.add_argument("--no-4bit", action="store_true")
    parser.add_argument("--hf-token", type=str, default=os.getenv("HF_TOKEN"))
    args = parser.parse_args()

    model, tokenizer = load_model_for_inference(
        base_model_name=args.base_model,
        adapter_path=args.adapter_path,
        use_4bit=not args.no_4bit,
        hf_token=args.hf_token,
    )

    dataset = load_from_disk(args.data_dir)
    split = dataset[args.split]

    evaluate_model(
        model=model,
        tokenizer=tokenizer,
        dataset_split=split,
        output_dir=args.output_dir,
        max_new_tokens=args.max_new_tokens,
        split_name=args.split,
    )


if __name__ == "__main__":
    main()
