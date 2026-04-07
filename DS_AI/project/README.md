# Hindi Riddle Solver — LLaMA-3 8B Fine-Tuned

> **Automated Riddle Solving in Indian Languages Using Deep Learning and Generative AI Techniques**  
> M.Tech DS/AI Project | Roll No: 206125030

---

## Problem Statement

Riddles in Indian languages such as **Hindi**, Tamil, Telugu, Malayalam, Kannada, and Bengali are rich in:
- Cultural metaphors and wordplay
- Phonetic cues and implicit reasoning
- Multi-step logical inference

This project fine-tunes **LLaMA-3 8B Instruct** on a curated Hindi riddle dataset using **QLoRA** (4-bit quantization + LoRA adapters) to create a system that can understand, reason over, and solve Hindi riddles.

---

## Architecture

```
Input Riddle (Hindi)
        ↓
LLaMA-3 8B Instruct (base)
  + QLoRA Fine-tuning (4-bit NF4 + LoRA r=16)
  + SFT on Hindi Riddle Dataset
        ↓
Answer + Explanation (Hindi)
```

### Key Components

| Component | Details |
|-----------|---------|
| **Base Model** | `meta-llama/Meta-Llama-3-8B-Instruct` |
| **Quantization** | BitsAndBytes 4-bit (NF4) |
| **PEFT** | LoRA (r=16, α=32) on q/k/v/o/gate/up/down projections |
| **Training** | TRL SFTTrainer with causal LM objective |
| **Prompting** | Zero-shot, Few-shot, Chain-of-Thought |

---

## Project Structure

```
project/
├── data_preparation.py     # Dataset builder (30+ seed riddles + augmentation)
├── train.py                # QLoRA fine-tuning script
├── evaluate.py             # Evaluation: Exact Match, BERTScore, ROUGE-L
├── inference.py            # Inference: CLI, Gradio UI, batch prediction
├── merge_model.py          # Merge LoRA adapter into base model
├── run_pipeline.sh         # End-to-end pipeline script
├── requirements.txt        # Python dependencies
├── .env.example            # Environment variables template
├── data/                   # (auto-generated) Dataset splits
│   ├── train.json
│   ├── val.json
│   └── test.json
├── outputs/                # (auto-generated) Model checkpoints
│   └── llama3-hindi-riddle-solver/
│       └── final_adapter/
└── eval_results/           # (auto-generated) Evaluation results & plots
```

---

## Setup

### 1. Environment

```bash
# Create virtual environment
python -m venv venv
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt
```

### 2. Configure

```bash
cp .env.example .env
# Edit .env and add your HuggingFace token
nano .env
```

> **Requirements for LLaMA-3:**
> - Accept the model license at: https://huggingface.co/meta-llama/Meta-Llama-3-8B-Instruct
> - Set `HF_TOKEN` in `.env`

### 3. GPU Requirements

| Mode | Min GPU VRAM |
|------|-------------|
| QLoRA fine-tuning (4-bit) | ~12 GB |
| Full precision fine-tuning | ~40 GB |
| Inference (4-bit) | ~6 GB |

---

## Usage

### Step 1: Prepare Dataset

```bash
python data_preparation.py \
    --output-dir ./data \
    --format llama3_chat \
    --augment-factor 2
```

### Step 2: Fine-tune

```bash
python train.py \
    --data-dir ./data \
    --output-dir ./outputs/llama3-hindi-riddle-solver \
    --epochs 5 \
    --batch-size 2 \
    --lr 2e-4 \
    --lora-r 16
```

### Step 3: Evaluate

```bash
python evaluate.py \
    --base-model meta-llama/Meta-Llama-3-8B-Instruct \
    --adapter-path ./outputs/llama3-hindi-riddle-solver/final_adapter \
    --data-dir ./data \
    --output-dir ./eval_results
```

### Step 4: Inference

```bash
# Interactive CLI
python inference.py --mode interactive

# Single riddle
python inference.py \
    --mode single \
    --riddle "दो भाई साथ-साथ चलते हैं, आमने-सामने कभी नहीं देखते।"

# Gradio Web UI
python inference.py --mode gradio --port 7860

# Batch from JSON file
python inference.py --mode batch --batch-file riddles.json
```

### Run Full Pipeline

```bash
bash run_pipeline.sh           # Full pipeline
bash run_pipeline.sh --demo    # With Gradio UI at end
bash run_pipeline.sh --skip-train  # Skip training (eval only)
```

---

## Dataset

The dataset includes **30+ authentic Hindi riddles** across 8 categories:

| Category     | Examples |
|--------------|---------|
| प्रकृति (Nature) | Stars, horizon, mountains, sunshine |
| शरीर (Body) | Eyes, fingers |
| वस्तुएं (Objects) | Comb, key, umbrella, time |
| खाना (Food) | Rice, watermelon, chili, betel leaf |
| दर्शन (Philosophy) | Age, distance, promise, shadow |
| शब्द-क्रीड़ा (Wordplay) | Name riddles, letter riddles |
| समय (Time) | Year/months/days |
| कृषि (Agriculture) | Fertilizer/compost |

Data augmentation using rephrasing (`~2×`) and the dataset is stored in **LLaMA-3 Chat Format** (instruct template).

---

## Evaluation Metrics

| Metric | Description |
|--------|-------------|
| **Exact Match** | Strict answer match (case-insensitive) |
| **BERTScore F1** | Semantic similarity (bert-base-multilingual-cased) |
| **ROUGE-L** | Longest common subsequence overlap |
| **BLEU** | n-gram precision |

Evaluation plots are generated automatically in `./eval_results/plots/`:
- Accuracy by category
- Accuracy by difficulty (easy / medium / hard)
- BERTScore F1 distribution

---

## Prompting Strategies

| Strategy | Description |
|----------|-------------|
| **Zero-shot** | Direct answer generation |
| **Few-shot** | 2 example riddles prepended |
| **Chain-of-Thought (CoT)** | Step-by-step reasoning before answer |

---

## References

1. **"The Riddle of Reflection"** — Evaluating Reasoning and Self-Awareness in Multilingual LLMs using Indian Riddles. ArXiv 2511.00960
2. Indian Riddles — A Forgotten Chapter in the Study of Indian Folklore. DLI Archive.
3. ACM DL — Multilingual Riddle Understanding and Reasoning.

---

## Training Details

| Parameter | Value |
|-----------|-------|
| LoRA rank (r) | 16 |
| LoRA alpha | 32 |
| LoRA dropout | 0.05 |
| Target modules | q, k, v, o, gate, up, down projections |
| Quantization | 4-bit NF4 (QLoRA) |
| Optimizer | Paged AdamW 32-bit |
| LR scheduler | Cosine |
| Warmup ratio | 0.03 |
| Max seq length | 512 tokens |
| Batch size | 2 × 4 gradient accum = 8 effective |
| Epochs | 5 |
| Learning rate | 2e-4 |

---

*M.Tech Data Science & AI — Project 206125030*
