import os
import json
import pandas as pd
from tqdm import tqdm
import unicodedata
import re
import time

# ====================== Configuration ======================
BASE_PATH = r"C:\Users\muham\OneDrive\Desktop\cord-19_2020-05-26\2020-05-26\document_parses\document_parses"
PMC_FOLDER = os.path.join(BASE_PATH, "pmc_json")
PDF_FOLDER = os.path.join(BASE_PATH, "pdf_json")
METADATA_FILE = r"C:\Users\muham\OneDrive\Desktop\cord-19_2020-05-26\2020-05-26\metadata.csv"
OUTPUT_FILE = "cord_processed.csv"

# ====================== Text Normalizer ======================
class TextNormalizer:
    def normalize(self, text):
        if not isinstance(text, str):
            return ""
        
        # ------------------ 1. Unicode normalization & remove accents ------------------
        text = unicodedata.normalize('NFKD', text)
        text = ''.join(c for c in text if not unicodedata.combining(c))

        # ------------------ 2. Lowercase ------------------
        text = text.lower()

        # ------------------ 3. Handle special sequences like emails/handles ------------------
        # separate @, #, etc with spaces
        text = re.sub(r'([@#])', r' \1 ', text)

        # ------------------ 4. Remove apostrophes ------------------
        text = text.replace("'", "")

        # ------------------ 5. Tokenize mathematical operators and parentheses ------------------
        text = re.sub(r"([+\-*/=<>])", r" \1 ", text)
        text = re.sub(r"([\(\)])", r" \1 ", text)

        # ------------------ 6. Tokenize dollar signs ------------------
        text = re.sub(r"\$", r" $ ", text)

        # ------------------ 7. Replace decimal points in numbers with space ------------------
        text = re.sub(r'\b(\d+)\.(\d+)\b', r'\1 \2', text)
        text = re.sub(r',', '', text)  # remove commas in numbers

        # ------------------ 8. Remove all other punctuation ------------------
        text = re.sub(r"[^a-z0-9\s+\-*/=<>()\$@#]", " ", text)

        # ------------------ 9. Collapse multiple spaces ------------------
        text = " ".join(text.split())

        return text

normalizer = TextNormalizer()

# ====================== Load existing processed cord_ids ======================
processed_ids = set()
if os.path.exists(OUTPUT_FILE):
    df_existing = pd.read_csv(OUTPUT_FILE)
    processed_ids.update(df_existing['cord_id'].astype(str).tolist())

# ====================== Load metadata ======================
metadata_df = pd.read_csv(METADATA_FILE, low_memory=False)
output_rows = []
start_time = time.time()

# ====================== Process JSON files ======================
for idx, row in tqdm(metadata_df.iterrows(), total=len(metadata_df), desc="Processing metadata"):
    cord_id = str(row.get('cord_uid', '')).strip()
    if not cord_id or cord_id in processed_ids:
        continue

    url = row.get('url', '')

    pdf_path = str(row.get('pdf_json_files', '')).strip() if pd.notna(row.get('pdf_json_files')) else ''
    pmc_path = str(row.get('pmc_json_files', '')).strip() if pd.notna(row.get('pmc_json_files')) else ''

    if not pmc_path and not pdf_path:
        continue

    pmc_filename = os.path.basename(pmc_path) if pmc_path else ''
    pdf_filename = os.path.basename(pdf_path) if pdf_path else ''

    json_path = os.path.join(PMC_FOLDER, pmc_filename) if pmc_filename else os.path.join(PDF_FOLDER, pdf_filename)
    json_path = os.path.normpath(json_path)

    if not os.path.exists(json_path):
        continue

    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        title = normalizer.normalize(data.get('metadata', {}).get('title', ''))

        abstract_list = data.get('abstract', [])
        abstract = ' '.join([normalizer.normalize(item.get('text', '')) for item in abstract_list])

        body_list = data.get('body_text', [])
        body_text = ' '.join([normalizer.normalize(item.get('text', '')) for item in body_list])

        authors_list = data.get('metadata', {}).get('authors', [])
        authors = ', '.join([normalizer.normalize(a.get('first', '') + ' ' + a.get('last', '')) for a in authors_list])

        journal = normalizer.normalize(data.get('metadata', {}).get('journal', ''))

        output_rows.append({
            'cord_id': cord_id,
            'url': url,
            'authors': authors,
            'title': title,
            'abstract': abstract,
            'body_text': body_text,
            'journal': journal
        })

        processed_ids.add(cord_id)

    except Exception as e:
        print(f"Error processing {json_path}: {e}")
        continue

# ====================== Write CSV ======================
if output_rows:
    output_df = pd.DataFrame(output_rows)
    if os.path.exists(OUTPUT_FILE):
        output_df.to_csv(OUTPUT_FILE, mode='a', header=False, index=False)
    else:
        output_df.to_csv(OUTPUT_FILE, index=False)

end_time = time.time()

print("\n========================================")
print(f"Processing completed!")
print(f"Total new papers processed: {len(output_rows)}")
print(f"CSV file saved to: {os.path.abspath(OUTPUT_FILE)}")
print(f"Total time: {end_time - start_time:.2f} seconds")
print("========================================\n")
