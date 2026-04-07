#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# run_pipeline.sh — Full pipeline: Data → Train → Evaluate → Demo
# Usage: bash run_pipeline.sh [--skip-train] [--demo]
# ─────────────────────────────────────────────────────────────────────────────

set -e

SKIP_TRAIN=false
LAUNCH_DEMO=false

# Parse args
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --skip-train) SKIP_TRAIN=true ;;
        --demo) LAUNCH_DEMO=true ;;
    esac
    shift
done

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo " Hindi Riddle Solver — LLaMA-3 8B Fine-Tuning Pipeline"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# ── Step 1: Data Preparation ──────────────────────────────────
echo ""
echo "[1/4] Preparing Hindi riddle dataset..."
python data_preparation.py \
    --output-dir ./data \
    --format llama3_chat \
    --augment-factor 2
echo "✓ Dataset prepared."

# ── Step 2: Training ──────────────────────────────────────────
if [ "$SKIP_TRAIN" = false ]; then
    echo ""
    echo "[2/4] Starting QLoRA fine-tuning..."
    python train.py \
        --data-dir ./data \
        --output-dir ./outputs/llama3-hindi-riddle-solver \
        --epochs 5 \
        --batch-size 2 \
        --lr 2e-4 \
        --lora-r 16 \
        --lora-alpha 32 \
        --max-seq-length 512
    echo "✓ Training complete."
else
    echo ""
    echo "[2/4] Skipping training (--skip-train flag set)."
fi

# ── Step 3: Evaluation ────────────────────────────────────────
echo ""
echo "[3/4] Evaluating on test set..."
python evaluate.py \
    --base-model "${MODEL_NAME:-meta-llama/Meta-Llama-3-8B-Instruct}" \
    --adapter-path ./outputs/llama3-hindi-riddle-solver/final_adapter \
    --data-dir ./data \
    --output-dir ./eval_results \
    --split test
echo "✓ Evaluation complete. Results in ./eval_results/"

# ── Step 4: Demo ──────────────────────────────────────────────
if [ "$LAUNCH_DEMO" = true ]; then
    echo ""
    echo "[4/4] Launching Gradio demo..."
    python inference.py \
        --base-model "${MODEL_NAME:-meta-llama/Meta-Llama-3-8B-Instruct}" \
        --adapter-path ./outputs/llama3-hindi-riddle-solver/final_adapter \
        --mode gradio \
        --port 7860
else
    echo ""
    echo "[4/4] Demo skipped. Run with --demo to launch Gradio UI."
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo " Pipeline complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
