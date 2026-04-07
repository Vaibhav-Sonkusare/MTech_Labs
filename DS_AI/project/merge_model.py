"""
Merge LoRA Adapters into the Base Model
Useful for:
  - Creating a standalone full model for deployment
  - Pushing the merged model to Hugging Face Hub
  - Faster inference (no PEFT overhead)
"""

import os
import argparse
import logging

import torch
from transformers import AutoTokenizer, AutoModelForCausalLM, BitsAndBytesConfig
from peft import PeftModel

logging.basicConfig(level=logging.INFO, format="%(asctime)s | %(message)s")
logger = logging.getLogger(__name__)


def merge_and_save(
    base_model_name: str,
    adapter_path: str,
    output_path: str,
    hf_token: str = None,
    push_to_hub: bool = False,
    hub_repo_id: str = None,
):
    logger.info(f"Base model  : {base_model_name}")
    logger.info(f"LoRA adapter: {adapter_path}")
    logger.info(f"Output path : {output_path}")

    # Load tokenizer
    tokenizer = AutoTokenizer.from_pretrained(
        base_model_name, token=hf_token, trust_remote_code=True
    )

    # Load base model in fp16 (no quantization for merging)
    logger.info("Loading base model in fp16 for merging...")
    model = AutoModelForCausalLM.from_pretrained(
        base_model_name,
        torch_dtype=torch.float16,
        device_map="auto",
        token=hf_token,
        trust_remote_code=True,
    )

    # Load LoRA adapter and merge
    logger.info("Loading and merging LoRA adapter...")
    model = PeftModel.from_pretrained(model, adapter_path)
    model = model.merge_and_unload()
    logger.info("Merge complete!")

    # Save merged model
    os.makedirs(output_path, exist_ok=True)
    model.save_pretrained(output_path)
    tokenizer.save_pretrained(output_path)
    logger.info(f"Merged model saved to: {output_path}")

    # Optionally push to Hub
    if push_to_hub and hub_repo_id:
        logger.info(f"Pushing to HuggingFace Hub: {hub_repo_id}")
        model.push_to_hub(hub_repo_id, token=hf_token)
        tokenizer.push_to_hub(hub_repo_id, token=hf_token)
        logger.info("Upload complete!")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Merge LoRA adapter into base LLaMA-3 model")
    parser.add_argument("--base-model", required=True)
    parser.add_argument("--adapter-path", required=True)
    parser.add_argument("--output-path", default="./merged_model")
    parser.add_argument("--hf-token", default=os.getenv("HF_TOKEN"))
    parser.add_argument("--push-to-hub", action="store_true")
    parser.add_argument("--hub-repo-id", type=str, default=None)
    args = parser.parse_args()

    merge_and_save(
        base_model_name=args.base_model,
        adapter_path=args.adapter_path,
        output_path=args.output_path,
        hf_token=args.hf_token,
        push_to_hub=args.push_to_hub,
        hub_repo_id=args.hub_repo_id,
    )
