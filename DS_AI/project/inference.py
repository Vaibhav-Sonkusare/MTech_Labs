"""
Inference / Demo Script for Hindi Riddle Solver
Supports:
  - Interactive CLI mode
  - Single riddle prediction
  - Batch prediction from file
  - Gradio web UI demo
"""

import os
import json
import argparse
import logging
from pathlib import Path
from typing import Optional, List

import torch
from transformers import AutoTokenizer, AutoModelForCausalLM, BitsAndBytesConfig
from peft import PeftModel

logging.basicConfig(level=logging.INFO, format="%(asctime)s | %(levelname)s | %(message)s")
logger = logging.getLogger(__name__)


# ─────────────────────────────────────────────────────────────────────────────
# Model Manager
# ─────────────────────────────────────────────────────────────────────────────

class HindiRiddleSolver:
    """
    A wrapper around LLaMA-3 (optionally fine-tuned) for solving Hindi riddles.
    Supports zero-shot, few-shot, and chain-of-thought prompting.
    """

    SYSTEM_PROMPT = (
        "आप एक विशेषज्ञ हिंदी पहेली-समाधानकर्ता और सांस्कृतिक ज्ञान विशेषज्ञ हैं। "
        "आपको हिंदी में दी गई पहेलियों का सटीक और सार्थक उत्तर देना है। "
        "अपना उत्तर इस प्रकार दें:\n"
        "**उत्तर:** [यहाँ उत्तर लिखें]\n\n"
        "**स्पष्टीकरण:** [यहाँ कारण बताएं]"
    )

    COT_SYSTEM_PROMPT = (
        "आप एक विशेषज्ञ हिंदी पहेली-समाधानकर्ता हैं। "
        "पहेली सुलझाने के लिए चरण-दर-चरण सोचें:\n"
        "1. पहेली में दिए गए संकेतों की पहचान करें\n"
        "2. प्रत्येक संकेत का अर्थ समझें\n"
        "3. सभी संकेतों को जोड़कर उत्तर तक पहुँचें\n"
        "अंत में स्पष्ट रूप से लिखें:\n"
        "**उत्तर:** [उत्तर]"
    )

    FEW_SHOT_EXAMPLES = [
        {
            "riddle": "दो भाई साथ-साथ चलते हैं, आमने-सामने कभी नहीं देखते।",
            "answer": "**उत्तर:** आँखें\n\n**स्पष्टीकरण:** दोनों आँखें एक-दूसरे को सीधे कभी नहीं देख सकतीं।"
        },
        {
            "riddle": "मेरे पास दाँत हैं पर मैं काट नहीं सकता।",
            "answer": "**उत्तर:** कंघी\n\n**स्पष्टीकरण:** कंघी में दाँत होते हैं पर वह काट नहीं सकती।"
        }
    ]

    def __init__(
        self,
        base_model_name: str = "meta-llama/Meta-Llama-3-8B-Instruct",
        adapter_path: Optional[str] = None,
        use_4bit: bool = True,
        hf_token: Optional[str] = None,
        device_map: str = "auto",
    ):
        self.base_model_name = base_model_name
        self.adapter_path = adapter_path
        self.device_map = device_map
        self._load_model(use_4bit, hf_token)

    def _load_model(self, use_4bit: bool, hf_token: Optional[str]):
        logger.info(f"Loading tokenizer: {self.base_model_name}")
        self.tokenizer = AutoTokenizer.from_pretrained(
            self.base_model_name,
            token=hf_token,
            trust_remote_code=True,
        )
        if self.tokenizer.pad_token is None:
            self.tokenizer.pad_token = self.tokenizer.eos_token
        self.tokenizer.padding_side = "left"

        bnb_config = None
        if use_4bit:
            bnb_config = BitsAndBytesConfig(
                load_in_4bit=True,
                bnb_4bit_quant_type="nf4",
                bnb_4bit_compute_dtype=torch.bfloat16,
            )

        logger.info("Loading model weights...")
        self.model = AutoModelForCausalLM.from_pretrained(
            self.base_model_name,
            quantization_config=bnb_config,
            device_map=self.device_map,
            token=hf_token,
            trust_remote_code=True,
        )

        if self.adapter_path and Path(self.adapter_path).exists():
            logger.info(f"Loading LoRA adapter: {self.adapter_path}")
            self.model = PeftModel.from_pretrained(self.model, self.adapter_path)
            self.model = self.model.merge_and_unload()
            logger.info("LoRA adapter merged.")

        self.model.eval()
        logger.info("Model ready for inference!")

    def _build_prompt(
        self,
        riddle: str,
        mode: str = "zero_shot",
        examples: Optional[List[dict]] = None
    ) -> str:
        """Build the prompt in LLaMA 3 chat format."""
        if mode == "cot":
            system = self.COT_SYSTEM_PROMPT
        else:
            system = self.SYSTEM_PROMPT

        messages = [
            {"role": "system", "content": system}
        ]

        # Add few-shot examples
        if mode == "few_shot":
            shot_examples = examples or self.FEW_SHOT_EXAMPLES
            for ex in shot_examples:
                messages.append({"role": "user", "content": f"पहेली: {ex['riddle']}"})
                messages.append({"role": "assistant", "content": ex["answer"]})

        # Add the actual riddle
        messages.append({
            "role": "user",
            "content": f"निम्नलिखित पहेली का उत्तर दीजिए:\n\n{riddle}"
        })

        # Build raw prompt string
        prompt = "<|begin_of_text|>"
        for msg in messages:
            role = msg["role"]
            content = msg["content"]
            prompt += (
                f"<|start_header_id|>{role}<|end_header_id|>\n\n"
                f"{content}<|eot_id|>"
            )
        prompt += "<|start_header_id|>assistant<|end_header_id|>\n\n"
        return prompt

    def solve(
        self,
        riddle: str,
        mode: str = "zero_shot",
        max_new_tokens: int = 250,
        temperature: float = 0.1,
        do_sample: bool = False,
        examples: Optional[List[dict]] = None,
    ) -> dict:
        """
        Solve a Hindi riddle.
        mode: 'zero_shot' | 'few_shot' | 'cot' (chain-of-thought)
        """
        prompt = self._build_prompt(riddle, mode=mode, examples=examples)
        inputs = self.tokenizer(
            prompt, return_tensors="pt", truncation=True, max_length=800
        ).to(self.model.device)

        with torch.no_grad():
            output_ids = self.model.generate(
                **inputs,
                max_new_tokens=max_new_tokens,
                temperature=temperature if do_sample else 1.0,
                do_sample=do_sample,
                pad_token_id=self.tokenizer.eos_token_id,
                eos_token_id=self.tokenizer.eos_token_id,
                repetition_penalty=1.1,
            )

        generated = output_ids[0][inputs["input_ids"].shape[1]:]
        full_response = self.tokenizer.decode(generated, skip_special_tokens=True).strip()

        # Extract answer
        answer = self._extract_answer(full_response)

        return {
            "riddle": riddle,
            "mode": mode,
            "answer": answer,
            "full_response": full_response,
        }

    def _extract_answer(self, response: str) -> str:
        """Extract the answer token from a full model response."""
        for line in response.split("\n"):
            if "उत्तर" in line and ":" in line:
                ans = line.split(":", 1)[-1].replace("**", "").strip()
                if ans:
                    return ans
        # Fallback: first non-empty line
        for line in response.split("\n"):
            line = line.strip().replace("**", "")
            if line:
                return line
        return response.strip()

    def batch_solve(
        self,
        riddles: List[str],
        mode: str = "zero_shot",
        **kwargs
    ) -> List[dict]:
        """Solve multiple riddles in batch."""
        return [self.solve(r, mode=mode, **kwargs) for r in riddles]


# ─────────────────────────────────────────────────────────────────────────────
# Gradio Web UI
# ─────────────────────────────────────────────────────────────────────────────

def launch_gradio(solver: HindiRiddleSolver, port: int = 7860):
    """Launch an interactive Gradio web demo."""
    try:
        import gradio as gr
    except ImportError:
        logger.error("Gradio not installed. Run: pip install gradio")
        return

    def solve_riddle(riddle, mode, max_tokens):
        if not riddle.strip():
            return "कृपया एक पहेली दर्ज करें।", ""
        result = solver.solve(riddle, mode=mode, max_new_tokens=int(max_tokens))
        return result["answer"], result["full_response"]

    with gr.Blocks(
        title="हिंदी पहेली समाधानकर्ता | Hindi Riddle Solver",
        theme=gr.themes.Soft(primary_hue="orange"),
        css="""
            .gradio-container { max-width: 860px; margin: auto; }
            h1 { text-align: center; font-size: 2em; }
            .description { text-align: center; color: #666; }
        """
    ) as demo:
        gr.Markdown(
            """
            # 🧩 हिंदी पहेली समाधानकर्ता
            ### Hindi Riddle Solver powered by LLaMA-3 8B (Fine-tuned)
            *Automated Riddle Solving in Indian Languages using Deep Learning & Generative AI*
            """
        )

        with gr.Row():
            with gr.Column(scale=2):
                riddle_input = gr.Textbox(
                    label="पहेली दर्ज करें (Enter Riddle in Hindi)",
                    placeholder="जैसे: दो भाई साथ-साथ चलते हैं, आमने-सामने कभी नहीं देखते।",
                    lines=4,
                    elem_id="riddle_input"
                )
                mode_radio = gr.Radio(
                    choices=["zero_shot", "few_shot", "cot"],
                    value="zero_shot",
                    label="Prompting Strategy",
                    info=(
                        "zero_shot: Direct answer | "
                        "few_shot: With examples | "
                        "cot: Chain-of-Thought reasoning"
                    )
                )
                max_tokens = gr.Slider(50, 400, value=200, step=50,
                                       label="Max response length (tokens)")
                solve_btn = gr.Button("पहेली सुलझाएं 🔍", variant="primary", size="lg")

            with gr.Column(scale=2):
                answer_output = gr.Textbox(
                    label="उत्तर (Answer)",
                    lines=2,
                    elem_id="answer_output"
                )
                full_output = gr.Textbox(
                    label="पूरा जवाब (Full Model Response)",
                    lines=8,
                    elem_id="full_output"
                )

        # Example riddles
        gr.Examples(
            examples=[
                ["दो भाई साथ-साथ चलते हैं, आमने-सामने कभी नहीं देखते।", "zero_shot", 200],
                ["बिना पंखों के उड़ता हूँ, बिना पैरों के भागता हूँ।", "few_shot", 200],
                ["सुबह चार पैर, दोपहर दो पैर, शाम तीन पैर।", "cot", 300],
                ["एक थाल मोती से भरा, सबके सर पर औंधा धरा।", "zero_shot", 200],
                ["हरे रंग का मेरा घर है, सफेद रंग के मेरे अंडे।", "few_shot", 200],
            ],
            inputs=[riddle_input, mode_radio, max_tokens],
            label="Sample Riddles / नमूना पहेलियाँ"
        )

        solve_btn.click(
            fn=solve_riddle,
            inputs=[riddle_input, mode_radio, max_tokens],
            outputs=[answer_output, full_output]
        )

        gr.Markdown(
            """
            ---
            **Model:** LLaMA-3 8B Instruct + QLoRA Fine-tuning on Hindi Riddles  
            **Techniques:** SFT, BERTScore evaluation, Chain-of-Thought prompting  
            **Reference:** [ArXiv 2511.00960](https://arxiv.org/abs/2511.00960)
            """
        )

    logger.info(f"Launching Gradio UI on port {port}")
    demo.launch(server_port=port, share=False, show_error=True)


# ─────────────────────────────────────────────────────────────────────────────
# Interactive CLI
# ─────────────────────────────────────────────────────────────────────────────

def interactive_cli(solver: HindiRiddleSolver):
    """Interactive command-line interface for riddle solving."""
    print("\n" + "=" * 60)
    print("  हिंदी पहेली समाधानकर्ता | Hindi Riddle Solver")
    print("  (Type 'quit' or 'exit' to stop)")
    print("=" * 60)

    modes = {"1": "zero_shot", "2": "few_shot", "3": "cot"}

    while True:
        print("\nमोड चुनें / Choose mode:")
        print("  1. Zero-Shot (सीधा उत्तर)")
        print("  2. Few-Shot (उदाहरण के साथ)")
        print("  3. Chain-of-Thought (चरण-दर-चरण)")
        mode_choice = input("→ ").strip()
        mode = modes.get(mode_choice, "zero_shot")

        riddle = input("\nपहेली दर्ज करें: ").strip()
        if riddle.lower() in ("quit", "exit", "q"):
            print("धन्यवाद! Goodbye!")
            break

        if not riddle:
            continue

        print("\n🔍 सोच रहे हैं...")
        result = solver.solve(riddle, mode=mode)

        print(f"\n✅ उत्तर: {result['answer']}")
        print(f"\n📝 पूरा जवाब:\n{result['full_response']}")
        print("─" * 60)


# ─────────────────────────────────────────────────────────────────────────────
# CLI Entry Point
# ─────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Hindi Riddle Solver - Inference & Demo"
    )
    parser.add_argument("--base-model", type=str,
                        default="meta-llama/Meta-Llama-3-8B-Instruct")
    parser.add_argument("--adapter-path", type=str, default=None)
    parser.add_argument("--hf-token", type=str, default=os.getenv("HF_TOKEN"))
    parser.add_argument("--no-4bit", action="store_true")
    parser.add_argument("--mode", type=str, default="interactive",
                        choices=["interactive", "gradio", "single", "batch"])
    parser.add_argument("--riddle", type=str, default=None,
                        help="Single riddle text (for --mode single)")
    parser.add_argument("--batch-file", type=str, default=None,
                        help="JSON file with list of riddles (for --mode batch)")
    parser.add_argument("--prompting", type=str, default="zero_shot",
                        choices=["zero_shot", "few_shot", "cot"])
    parser.add_argument("--port", type=int, default=7860)
    args = parser.parse_args()

    solver = HindiRiddleSolver(
        base_model_name=args.base_model,
        adapter_path=args.adapter_path,
        use_4bit=not args.no_4bit,
        hf_token=args.hf_token,
    )

    if args.mode == "interactive":
        interactive_cli(solver)

    elif args.mode == "gradio":
        launch_gradio(solver, port=args.port)

    elif args.mode == "single":
        if not args.riddle:
            print("Error: --riddle is required for single mode")
            return
        result = solver.solve(args.riddle, mode=args.prompting)
        print(f"\n✅ उत्तर: {result['answer']}")
        print(f"\n📝 Full response:\n{result['full_response']}")

    elif args.mode == "batch":
        if not args.batch_file:
            print("Error: --batch-file is required for batch mode")
            return
        with open(args.batch_file, "r", encoding="utf-8") as f:
            riddles = json.load(f)
        if isinstance(riddles[0], dict):
            riddle_texts = [r["riddle"] for r in riddles]
        else:
            riddle_texts = riddles
        results = solver.batch_solve(riddle_texts, mode=args.prompting)
        out_path = args.batch_file.replace(".json", "_predictions.json")
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(results, f, ensure_ascii=False, indent=2)
        print(f"Predictions saved to: {out_path}")


if __name__ == "__main__":
    main()
