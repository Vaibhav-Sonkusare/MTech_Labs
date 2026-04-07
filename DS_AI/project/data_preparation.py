"""
Hindi Riddle Dataset Preparation and Collection Script
Supports:
  - Loading riddles from CSV/JSON
  - Generating synthetic Hindi riddles using templates
  - Augmenting with context-reconstructed variants (inspired by RISCORE framework)
  - Splitting into train/val/test sets and saving to HuggingFace Dataset format
"""

import os
import json
import random
import re
import pandas as pd
from pathlib import Path
from datasets import Dataset, DatasetDict
from typing import List, Dict, Optional
from sklearn.model_selection import train_test_split


# ─────────────────────────────────────────────────────────────────────────────
# Seed Hindi Riddle Data (पहेलियाँ)
# These are authentic, culturally grounded Hindi riddles with answers.
# Each riddle has: riddle text, answer, explanation, category, difficulty
# ─────────────────────────────────────────────────────────────────────────────

HINDI_RIDDLES_SEED: List[Dict] = [
    # Nature / प्रकृति
    {
        "riddle": "हरे रंग का मेरा घर है, सफेद रंग के मेरे अंडे, न मैं मुर्गी, न बत्तख, बताओ मेरा नाम।",
        "answer": "नारियल",
        "explanation": "नारियल पेड़ पर हरे रंग के छिलके में होता है और अंदर सफेद गूदा होता है।",
        "category": "प्रकृति",
        "difficulty": "easy"
    },
    {
        "riddle": "एक चीज़ जिसे तुम देख सकते हो पर छू नहीं सकते, उसे पाने के लिए जितना दौड़ो, उतना दूर जाती है।",
        "answer": "क्षितिज",
        "explanation": "क्षितिज वह रेखा है जहाँ आकाश और धरती मिलते दिखते हैं, पर वास्तव में कभी मिलते नहीं।",
        "category": "प्रकृति",
        "difficulty": "medium"
    },
    {
        "riddle": "रात को जन्म, सुबह मरण, फिर भी रोज़ आता हूँ।",
        "answer": "तारा",
        "explanation": "तारे रात को दिखाई देते हैं और सूरज उगने पर दिखना बंद हो जाते हैं।",
        "category": "प्रकृति",
        "difficulty": "easy"
    },
    {
        "riddle": "न हाथ, न पैर, फिर भी सबसे ऊँचे पहाड़ चढ़ जाता हूँ।",
        "answer": "धूप / सूर्य की किरण",
        "explanation": "सूर्य की धूप बिना किसी शारीरिक अंग के ऊँचे से ऊँचे पहाड़ पर पहुँच जाती है।",
        "category": "प्रकृति",
        "difficulty": "medium"
    },
    {
        "riddle": "जितना निकालो उतना बढ़ता है, बिना मेहनत के भरा रहता है।",
        "answer": "गड्ढा / खाई",
        "explanation": "जितनी मिट्टी खोदते हैं, गड्ढा उतना ही गहरा और बड़ा होता जाता है।",
        "category": "प्रकृति",
        "difficulty": "easy"
    },
    {
        "riddle": "बिना बुलाए आता हूँ, बिना विदाई के जाता हूँ, महीनों रहता हूँ पर कोई देखता नहीं।",
        "answer": "साँस / वायु",
        "explanation": "हवा बिना बुलाए आती-जाती है, हर पल साथ रहती है पर दिखती नहीं।",
        "category": "प्रकृति",
        "difficulty": "hard"
    },

    # Body / शरीर
    {
        "riddle": "दो भाई साथ-साथ चलते हैं, आमने-सामने कभी नहीं देखते।",
        "answer": "आँखें",
        "explanation": "दोनों आँखें एक-दूसरे को सीधे नहीं देख सकती, हमेशा एक ही दिशा में देखती हैं।",
        "category": "शरीर",
        "difficulty": "easy"
    },
    {
        "riddle": "पाँच भाइयों एक घर में रहते हैं, सब एक साथ नाचते हैं।",
        "answer": "उँगलियाँ",
        "explanation": "हाथ की पाँच उँगलियाँ एक हाथ में रहती हैं और एक साथ काम करती हैं।",
        "category": "शरीर",
        "difficulty": "easy"
    },
    {
        "riddle": "खाओ तो बढ़ता है, न खाओ तो घटता है, पर वज़न शून्य है उसका।",
        "answer": "उम्र",
        "explanation": "उम्र खाने (समय बिताने) से बढ़ती है लेकिन इसका कोई वज़न नहीं होता।",
        "category": "जीवन",
        "difficulty": "hard"
    },
    {
        "riddle": "बिना माँगे सब देता हूँ, पेड़ों को जीवन, खेतों को सोना।",
        "answer": "बारिश / वर्षा",
        "explanation": "बारिश बिना माँगे पेड़-पौधों को पानी देती है और खेतों को फसल उगाने में मदद करती है।",
        "category": "प्रकृति",
        "difficulty": "easy"
    },

    # Objects / वस्तुएं
    {
        "riddle": "मेरे पास दाँत हैं पर मैं काट नहीं सकता, रोज़ तुम्हारे काम आता हूँ।",
        "answer": "कंघी",
        "explanation": "कंघी में दाँत होते हैं पर वह काट नहीं सकती, बालों को सुलझाने के काम आती है।",
        "category": "वस्तुएं",
        "difficulty": "easy"
    },
    {
        "riddle": "सफर में साथी, घर में बंदी, जाने का नाम नहीं।",
        "answer": "चाबी",
        "explanation": "चाबी यात्रा (सफर) में साथ रखते हैं और घर में ताला बंद करती है।",
        "category": "वस्तुएं",
        "difficulty": "medium"
    },
    {
        "riddle": "बिना पंखों के उड़ता हूँ, बिना पैरों के भागता हूँ।",
        "answer": "समय / वक्त",
        "explanation": "समय न पंखों से उड़ता है न पैरों से चलता है, फिर भी हमेशा आगे बढ़ता रहता है।",
        "category": "दर्शन",
        "difficulty": "medium"
    },
    {
        "riddle": "काला घोड़ा, सफेद मैदान, उस पर छोटे-छोटे सवार।",
        "answer": "कागज़ और कलम से लिखाई",
        "explanation": "काली स्याही (घोड़ा), सफेद कागज़ (मैदान), और अक्षर (सवार) लिखाई को दर्शाते हैं।",
        "category": "शिक्षा",
        "difficulty": "medium"
    },
    {
        "riddle": "जितना पुराना उतना कीमती, जितना नया उतना सस्ता।",
        "answer": "शराब / पुरानी चीज़ें जैसे सोना",
        "explanation": "शराब जितनी पुरानी होती है उतनी महंगी होती है, जबकि नई सस्ती होती है।",
        "category": "वस्तुएं",
        "difficulty": "hard"
    },

    # Food / खाना
    {
        "riddle": "आग में पका, पानी में उगा, धरती का पुत्र, पेट का राजा।",
        "answer": "चावल",
        "explanation": "चावल पानी में उगता है (धान के खेत में), फिर आग पर पकाया जाता है और भोजन का मुख्य अंग है।",
        "category": "खाना",
        "difficulty": "easy"
    },
    {
        "riddle": "लाल रंग की मेरी काया, मीठा-तीखा मेरा स्वभाव, खाने में रंग भरता हूँ।",
        "answer": "मिर्च",
        "explanation": "लाल मिर्च लाल रंग की होती है और भोजन में तीखापन और रंग दोनों देती है।",
        "category": "खाना",
        "difficulty": "easy"
    },

    # Culture / संस्कृति
    {
        "riddle": "सात रंग, एक आकृति, बारिश के बाद आती हूँ, बच्चों को लुभाती हूँ।",
        "answer": "इंद्रधनुष",
        "explanation": "इंद्रधनुष में सात रंग होते हैं और बारिश के बाद आकाश में दिखता है।",
        "category": "प्रकृति",
        "difficulty": "easy"
    },
    {
        "riddle": "वह क्या है जो तुम दे सकते हो पर खुद भी रख सकते हो?",
        "answer": "वादा / शब्द",
        "explanation": "एक वादा या शब्द आप दूसरे को दे सकते हैं और खुद भी उसे याद रख सकते हैं।",
        "category": "दर्शन",
        "difficulty": "medium"
    },
    {
        "riddle": "एक माँ की बारह बेटियाँ, हर बेटी के तीस बेटे।",
        "answer": "साल, महीने और दिन",
        "explanation": "एक साल में बारह महीने होते हैं और हर महीने में लगभग तीस दिन होते हैं।",
        "category": "समय",
        "difficulty": "hard"
    },
    {
        "riddle": "ऊपर से हरा, नीचे से लाल, बीच में सफेद रसीला।",
        "answer": "तरबूज",
        "explanation": "तरबूज बाहर से हरा, अंदर से लाल और उसमें सफेद बीज होते हैं, यह रसदार फल है।",
        "category": "खाना",
        "difficulty": "easy"
    },
    {
        "riddle": "बिना जड़ के पेड़, बिना पानी के उगे, पर इसकी छाया में दुनिया बसती है।",
        "answer": "छाता",
        "explanation": "छाता पेड़ जैसा दिखता है पर इसकी जड़ें नहीं होतीं, बारिश-धूप से बचाता है।",
        "category": "वस्तुएं",
        "difficulty": "medium"
    },
    {
        "riddle": "मेरी शुरुआत मृत्यु से होती है, पर मैं जीवन देता हूँ।",
        "answer": "खाद / उर्वरक",
        "explanation": "खाद मृत जीवों और पौधों से बनती है पर नए पौधों को जीवन देती है।",
        "category": "कृषि",
        "difficulty": "hard"
    },
    {
        "riddle": "न सोता हूँ, न जागता हूँ, हमेशा एक ही जगह रहता हूँ।",
        "answer": "पहाड़",
        "explanation": "पहाड़ हमेशा स्थिर रहता है, न सोता है न जागता है।",
        "category": "प्रकृति",
        "difficulty": "easy"
    },
    {
        "riddle": "जो तुम्हारे साथ है पर तुम कभी देख नहीं सकते।",
        "answer": "परछाई",
        "explanation": "परछाई हमेशा साथ रहती है पर सामने आकर कभी खुद को नहीं दिखाती।",
        "category": "दर्शन",
        "difficulty": "medium"
    },
    {
        "riddle": "आगे जाने पर घटती है, पीछे आने पर बढ़ती है।",
        "answer": "दूरी",
        "explanation": "मंज़िल की तरफ जाने पर दूरी घटती है और लौटने पर बढ़ती है।",
        "category": "दर्शन",
        "difficulty": "medium"
    },
    {
        "riddle": "न खाता, न पीता, फिर भी जिंदा रहता है।",
        "answer": "आग",
        "explanation": "आग बिना खाए-पीए जलती रहती है (ऑक्सीजन से जलती है पर खाना-पीना नहीं)।",
        "category": "प्रकृति",
        "difficulty": "easy"
    },
    {
        "riddle": "एक थाल मोती से भरा, सबके सर पर औंधा धरा।",
        "answer": "आकाश और तारे",
        "explanation": "रात का आकाश थाल जैसा दिखता है और तारे मोती जैसे जड़े हैं।",
        "category": "प्रकृति",
        "difficulty": "medium"
    },
    {
        "riddle": "हरा था मैदान, काटने वाले आए, लाल हो गया मैदान।",
        "answer": "पान",
        "explanation": "पान का पत्ता हरा होता है, चबाने पर लाल हो जाता है।",
        "category": "खाना",
        "difficulty": "medium"
    },
    {
        "riddle": "दो अक्षर का नाम है मेरा, पहले अक्षर में बल है, दूसरे में काम है।",
        "answer": "बलराम",
        "explanation": "बल + राम = बलराम, एक भारतीय नाम जिसमें शक्ति और भक्ति दोनों हैं।",
        "category": "शब्द-क्रीड़ा",
        "difficulty": "hard"
    },
    {
        "riddle": "सुबह चार पैर, दोपहर दो पैर, शाम तीन पैर।",
        "answer": "मनुष्य",
        "explanation": "बचपन में चार (हाथ-पैर से रेंगना), जवानी में दो पैर, बुढ़ापे में छड़ी सहित तीन।",
        "category": "दर्शन",
        "difficulty": "medium"
    },
]


def create_alpaca_format(riddle_dict: Dict) -> Dict:
    """Convert a riddle to Alpaca instruction-following format for SFT."""
    instruction = (
        "आप एक विशेषज्ञ हिंदी पहेली-समाधानकर्ता हैं। "
        "नीचे दी गई पहेली को ध्यान से पढ़ें और सही उत्तर दें। "
        "उत्तर के बाद एक संक्षिप्त स्पष्टीकरण भी दें।"
    )
    input_text = f"पहेली: {riddle_dict['riddle']}"
    output_text = (
        f"उत्तर: {riddle_dict['answer']}\n\n"
        f"स्पष्टीकरण: {riddle_dict['explanation']}"
    )
    return {
        "instruction": instruction,
        "input": input_text,
        "output": output_text,
        "riddle": riddle_dict["riddle"],
        "answer": riddle_dict["answer"],
        "explanation": riddle_dict["explanation"],
        "category": riddle_dict.get("category", "सामान्य"),
        "difficulty": riddle_dict.get("difficulty", "medium"),
        "language": "hindi"
    }


def create_llama3_chat_format(riddle_dict: Dict) -> Dict:
    """
    Format riddles using LLaMA 3 chat template for supervised fine-tuning.
    Uses the special tokens: <|begin_of_text|>, <|start_header_id|>, etc.
    """
    system_msg = (
        "आप एक विशेषज्ञ हिंदी पहेली-समाधानकर्ता और सांस्कृतिक ज्ञान विशेषज्ञ हैं। "
        "आपको हिंदी में दी गई पहेलियों का सटीक और सार्थक उत्तर देना है। "
        "उत्तर हमेशा हिंदी में दें और एक छोटा सा स्पष्टीकरण भी जोड़ें।"
    )
    user_msg = f"निम्नलिखित पहेली का उत्तर दीजिए:\n\n{riddle_dict['riddle']}"
    assistant_msg = (
        f"**उत्तर:** {riddle_dict['answer']}\n\n"
        f"**स्पष्टीकरण:** {riddle_dict['explanation']}"
    )

    # Build full text in LLaMA 3 chat format
    full_text = (
        "<|begin_of_text|>"
        "<|start_header_id|>system<|end_header_id|>\n\n"
        f"{system_msg}<|eot_id|>"
        "<|start_header_id|>user<|end_header_id|>\n\n"
        f"{user_msg}<|eot_id|>"
        "<|start_header_id|>assistant<|end_header_id|>\n\n"
        f"{assistant_msg}<|eot_id|>"
    )

    return {
        "text": full_text,
        "instruction": system_msg,
        "input": user_msg,
        "output": assistant_msg,
        "riddle": riddle_dict["riddle"],
        "answer": riddle_dict["answer"],
        "explanation": riddle_dict["explanation"],
        "category": riddle_dict.get("category", "सामान्य"),
        "difficulty": riddle_dict.get("difficulty", "medium"),
        "language": "hindi"
    }


def augment_riddles(riddles: List[Dict], n_augmented: int = 3) -> List[Dict]:
    """
    Simple data augmentation by rephrasing riddles.
    In practice, you'd use an LLM for context-reconstruction (RISCORE style).
    This creates variations with alternative question formulations.
    """
    augmented = []
    question_starters = [
        "बताओ, वह क्या है जो — ",
        "पहेली: ",
        "सोचो और बताओ — ",
        "यह क्या है? ",
        "जवाब दो — "
    ]
    for r in riddles:
        for i in range(min(n_augmented, len(question_starters))):
            # Only add starter prefix as a simple augmentation
            new_riddle = r.copy()
            starter = question_starters[i % len(question_starters)]
            if not r["riddle"].startswith(starter.strip()):
                new_riddle["riddle"] = starter + r["riddle"]
                new_riddle["augmented"] = True
                augmented.append(new_riddle)
    return augmented


def load_external_riddles(filepath: str) -> List[Dict]:
    """
    Load riddles from external CSV or JSON file.
    Expected CSV columns: riddle, answer, explanation, category, difficulty
    """
    path = Path(filepath)
    if not path.exists():
        print(f"[WARNING] File {filepath} not found. Skipping.")
        return []

    if path.suffix == ".csv":
        df = pd.read_csv(filepath)
        required_cols = ["riddle", "answer"]
        if not all(c in df.columns for c in required_cols):
            raise ValueError(f"CSV must have columns: {required_cols}")
        df["explanation"] = df.get("explanation", "")
        df["category"] = df.get("category", "सामान्य")
        df["difficulty"] = df.get("difficulty", "medium")
        return df.to_dict("records")

    elif path.suffix == ".json":
        with open(filepath, "r", encoding="utf-8") as f:
            return json.load(f)
    else:
        raise ValueError(f"Unsupported file format: {path.suffix}")


def prepare_dataset(
    seed_riddles: List[Dict],
    external_path: Optional[str] = None,
    use_augmentation: bool = True,
    augment_factor: int = 2,
    test_size: float = 0.15,
    val_size: float = 0.10,
    output_dir: str = "./data",
    format_type: str = "llama3_chat",   # or "alpaca"
    seed: int = 42
) -> DatasetDict:
    """
    Full pipeline to prepare the Hindi riddle dataset.
    """
    random.seed(seed)
    os.makedirs(output_dir, exist_ok=True)

    # 1. Start with seed riddles
    all_riddles = seed_riddles.copy()
    print(f"[INFO] Seed riddles: {len(all_riddles)}")

    # 2. Load external riddles if provided
    if external_path:
        external = load_external_riddles(external_path)
        all_riddles.extend(external)
        print(f"[INFO] After external data: {len(all_riddles)} riddles")

    # 3. Augment
    if use_augmentation:
        augmented = augment_riddles(all_riddles, n_augmented=augment_factor)
        all_riddles.extend(augmented)
        print(f"[INFO] After augmentation: {len(all_riddles)} riddles")

    # 4. Remove duplicates
    seen = set()
    unique_riddles = []
    for r in all_riddles:
        key = r["riddle"].strip()
        if key not in seen:
            seen.add(key)
            unique_riddles.append(r)
    print(f"[INFO] Unique riddles: {len(unique_riddles)}")

    # 5. Format
    format_fn = (
        create_llama3_chat_format if format_type == "llama3_chat"
        else create_alpaca_format
    )
    formatted = [format_fn(r) for r in unique_riddles]

    # 6. Train / val / test split
    train_val, test = train_test_split(
        formatted, test_size=test_size, random_state=seed
    )
    train, val = train_test_split(
        train_val, test_size=val_size / (1 - test_size), random_state=seed
    )
    print(f"[INFO] Split → Train: {len(train)} | Val: {len(val)} | Test: {len(test)}")

    # 7. Build HuggingFace DatasetDict
    dataset_dict = DatasetDict({
        "train": Dataset.from_list(train),
        "validation": Dataset.from_list(val),
        "test": Dataset.from_list(test)
    })

    # 8. Save to disk
    dataset_dict.save_to_disk(output_dir)
    print(f"[INFO] Dataset saved to {output_dir}")

    # 9. Also save as JSON for inspection
    for split, data in zip(["train", "val", "test"], [train, val, test]):
        json_path = os.path.join(output_dir, f"{split}.json")
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    print(f"[INFO] JSON splits saved to {output_dir}")

    return dataset_dict


def main():
    import argparse
    parser = argparse.ArgumentParser(
        description="Prepare Hindi riddle dataset for LLaMA-3 fine-tuning"
    )
    parser.add_argument("--external", type=str, default=None,
                        help="Path to external CSV/JSON riddle file")
    parser.add_argument("--output-dir", type=str, default="./data",
                        help="Output directory for the dataset")
    parser.add_argument("--format", type=str, default="llama3_chat",
                        choices=["llama3_chat", "alpaca"],
                        help="Output format type")
    parser.add_argument("--no-augment", action="store_true",
                        help="Disable data augmentation")
    parser.add_argument("--augment-factor", type=int, default=2,
                        help="Number of augmented variants per riddle")
    args = parser.parse_args()

    dataset = prepare_dataset(
        seed_riddles=HINDI_RIDDLES_SEED,
        external_path=args.external,
        use_augmentation=not args.no_augment,
        augment_factor=args.augment_factor,
        output_dir=args.output_dir,
        format_type=args.format,
    )

    print("\n[SUCCESS] Dataset preparation complete!")
    print(f"  Train samples  : {len(dataset['train'])}")
    print(f"  Val samples    : {len(dataset['validation'])}")
    print(f"  Test samples   : {len(dataset['test'])}")
    print(f"\nSample training entry:")
    print(dataset["train"][0]["text"][:500] + "...")


if __name__ == "__main__":
    main()
