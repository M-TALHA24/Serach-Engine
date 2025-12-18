# 🔬 CORD-19 Search Engine

A full-stack search engine for the COVID-19 Open Research Dataset (CORD-19), built with C++ backend and React frontend.

## Features

- **BM25 Semantic Search** - Industry-standard ranking algorithm with TF-IDF
- **Autocomplete** - Real-time word suggestions using Trie data structure
- **Inverted Index** - Fast document retrieval with barreled posting lists
- **Modern UI** - Responsive React frontend with search modes (AND/OR)

## Architecture

```
┌─────────────────────┐         ┌─────────────────────┐
│   React Frontend    │  HTTP   │   C++ API Server    │
│   (TypeScript)      │◄───────►│   (Winsock)         │
│   Port 5173         │         │   Port 5000         │
└─────────────────────┘         └──────────┬──────────┘
                                           │
                                           ▼
                                ┌─────────────────────┐
                                │   Indexed Data      │
                                │   (CSV files)       │
                                └─────────────────────┘
```

## Data Structures

| Structure          | Purpose                   | File                     |
| ------------------ | ------------------------- | ------------------------ |
| **Trie**           | Prefix-based autocomplete | `src/trie.cpp`           |
| **Lexicon**        | Word → ID mapping         | `src/lexicon.cpp`        |
| **Inverted Index** | Word → Documents          | `src/inverted_index.cpp` |
| **Forward Index**  | Document → Words          | `src/forward_index.cpp`  |
| **Barrels**        | Sharded posting lists     | `data/barrels/`          |

## Project Structure

```
Serach-Engine/
├── src/                    # Core C++ source files
│   ├── api_server.cpp      # HTTP server with search & autocomplete
│   ├── indexer_main.cpp    # Document indexing pipeline
│   ├── search_main.cpp     # CLI search interface
│   ├── lexicon.cpp/h       # Word-ID dictionary with Trie
│   ├── inverted_index.cpp  # Barrel-based posting lists
│   ├── trie.cpp/h          # Autocomplete data structure
│   └── tokenizer.cpp/h     # Text tokenization
├── frontend/               # React + Vite frontend
│   └── src/
│       ├── App.tsx         # Main application
│       └── components/     # SearchBar, SearchResults
├── data/                   # Indexed data
│   ├── lexicon.csv         # Word vocabulary
│   ├── postings.csv        # Merged posting lists
│   ├── barrels/            # Sharded inverted index
│   └── hitlists/           # Word positions & priorities
└── data_to_info.py         # Python data preprocessor
```

## Setup

### Prerequisites

- **C++ Compiler** (g++ with C++17 support)
- **Node.js** (v18+)
- **CORD-19 Dataset** (optional, for re-indexing)

### Build & Run

1. **Compile the API Server**

   ```bash
   g++ -std=c++17 -O2 -o api_server.exe src/api_server.cpp -lws2_32
   ```

2. **Start the Backend**

   ```bash
   ./api_server.exe
   ```

3. **Start the Frontend**

   ```bash
   cd frontend
   npm install
   npm run dev
   ```

4. **Open** http://localhost:5173

## API Endpoints

| Endpoint                   | Method | Description                              |
| -------------------------- | ------ | ---------------------------------------- |
| `/search?q=<query>`        | GET    | Search documents, returns ranked results |
| `/autocomplete?q=<prefix>` | GET    | Get word suggestions for prefix          |

### Example Response

```json
{
  "results": [
    {
      "docId": "abc123",
      "title": "COVID-19 Vaccine Efficacy Study",
      "authors": "Smith, J.",
      "abstract": "This study examines...",
      "score": 12.45
    }
  ]
}
```

## Search Algorithm (BM25)

The search uses **BM25** ranking with:

- **IDF** - Rare terms weighted higher than common ones
- **Term Saturation** (k1=1.5) - Prevents keyword stuffing
- **Length Normalization** (b=0.75) - Fair comparison across document sizes
- **Coordination Factor** - Boosts documents matching multiple query terms

## Indexing Pipeline

1. **Preprocess** - `data_to_info.py` extracts text from CORD-19 JSON
2. **Tokenize** - Split text into normalized words
3. **Build Lexicon** - Assign unique ID to each word
4. **Create Barrels** - Shard inverted index by word ID
5. **Merge Postings** - Combine barrels into final index

```bash
# Rebuild index (requires CORD-19 data)
g++ -std=c++17 -o indexer.exe src/indexer_main.cpp src/*.cpp
./indexer.exe
```

## Tech Stack

- **Backend**: C++17, Winsock2, BM25
- **Frontend**: React 19, TypeScript, Vite
- **Data**: CORD-19 COVID-19 Research Dataset

## License

Academic project - DSA Course (3rd Semester)
