#include <bits/stdc++.h>
#include <filesystem>
#include <nlohmann/json.hpp> // JSON library (nlohmann)
using json = nlohmann::json;

using namespace std;
namespace fs = std::filesystem;

const int BARREL_SIZE = 1000; // words per barrel

// ---------------- STRUCTS ----------------
struct WordInfo {
    vector<int> positions;
    int priority; // 1=title,2=abstract,3=body
};

// ---------------- GLOBALS ----------------
unordered_map<string,int> lexicon; // word -> wordID
int nextWordID = 0;

// ---------------- UTIL - CLEAN & TOKENIZE ----------------
string clean(const string &s) {
    string res = s;
    for (auto &c : res) {
        if (!isalpha((unsigned char)c)) c = ' ';
        else c = tolower((unsigned char)c);
    }
    return res;
}

vector<string> tokenize(const string &text) {
    vector<string> words;
    stringstream ss(clean(text));
    string w;
    while (ss >> w) words.push_back(w);
    return words;
}

// ---------------- LEXICON IO ----------------
void loadLexiconIfExists(const string &path) {
    ifstream fin(path);
    if (!fin.is_open()) return;
    string header;
    getline(fin, header);
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string word; int id;
        getline(ss, word, ',');
        ss >> id;
        lexicon[word] = id;
        nextWordID = max(nextWordID, id + 1);
    }
    fin.close();
}

void saveLexicon(const string &path) {
    ofstream out(path);
    out << "word,wordID\n";
    for (auto &p : lexicon) {
        out << p.first << "," << p.second << "\n";
    }
    out.close();
}

// ---------------- FILE APPEND HELPERS ----------------
void appendLine(const string &path, const string &line) {
    ofstream out(path, ios::app);
    out << line << "\n";
}

// ---------------- GET/ASSIGN WORD ID ----------------
int getWordID(const string &w) {
    auto it = lexicon.find(w);
    if (it != lexicon.end()) return it->second;
    int id = nextWordID++;
    lexicon[w] = id;
    return id;
}

int getBarrelID(int wordID) {
    return wordID / BARREL_SIZE;
}

// ---------------- PROCESS A SINGLE DOCUMENT ----------------
void processDocument(const string &docID, const string &title,
                     const string &abstractText, const string &body) {

    unordered_map<int, WordInfo> wordData; // per-document postings

    auto addSection = [&](const string &text, int priority) {
        vector<string> words = tokenize(text);
        for (int pos = 0; pos < (int)words.size(); ++pos) {
            const string &w = words[pos];
            if (w.empty()) continue;
            int wid = getWordID(w);
            wordData[wid].positions.push_back(pos);
            wordData[wid].priority = priority;
        }
    };

    addSection(title, 1);
    addSection(abstractText, 2);
    addSection(body, 3);

    // write forward index line (optional helpful debug)
    // format: docID,wordID:freq;wordID:freq;...
    {
        string fpath = "data/forward_index.csv";
        ofstream fout(fpath, ios::app);
        fout << docID << ",";
        bool first = true;
        for (auto &kv : wordData) {
            if (!first) fout << ";";
            first = false;
            fout << kv.first << ":" << kv.second.positions.size();
        }
        fout << "\n";
        fout.close();
    }

    // write barrels and hitlists per word in this document
    for (auto &kv : wordData) {
        int wordID = kv.first;
        int freq = (int)kv.second.positions.size();
        int priority = kv.second.priority;
        int barrelID = getBarrelID(wordID);

        string barrelPath = "data/barrels/barrel_" + to_string(barrelID) + ".csv";
        string hitlistPath = "data/hitlists/hitlist_" + to_string(barrelID) + ".csv";

        // barrel: wordID,docID,freq
        appendLine(barrelPath, to_string(wordID) + "," + docID + "," + to_string(freq));

        // hitlist: wordID,docID,freq,priority,pos1|pos2|...
        string poslist;
        for (int i = 0; i < (int)kv.second.positions.size(); ++i) {
            if (i) poslist += "|";
            poslist += to_string(kv.second.positions[i]);
        }
        appendLine(hitlistPath,
                   to_string(wordID) + "," + docID + "," + to_string(freq) + "," +
                   to_string(priority) + "," + poslist);
    }
}

// ----------------- SAFE CSV SPLIT FOR METADATA (handles quotes) -----------------
// Splits a CSV line into fields, respecting quoted commas.
vector<string> splitCSVLine(const string &line) {
    vector<string> cols;
    string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' ) {
            inQuotes = !inQuotes;
            // skip adding quote to field
        } else if (c == ',' && !inQuotes) {
            cols.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    cols.push_back(cur);
    return cols;
}

// ----------------- AGGREGATE POSTINGS PER BARREL (write postings.csv) -----------------
// For each barrel id present in data/barrels, read corresponding hitlist file and
// build postings entries for the words in that barrel and append to postings.csv
void buildPostingsFromHitlists(const string &barrelDir, const string &hitlistDir, const string &outPostingsPath) {
    // Ensure postings file has header
    ofstream pout(outPostingsPath);
    pout << "wordID,docIDs,freqPerDoc,priority,totalFrequency\n";
    pout.close();

    // iterate barrel files (we'll iterate hitlist files because they contain priority & positions)
    for (auto &entry : fs::directory_iterator(hitlistDir)) {
        if (!entry.is_regular_file()) continue;
        string hitlistPath = entry.path().string();
        // expect filename hitlist_X.csv => extract X
        string filename = entry.path().filename().string();
        // open and read all lines, aggregate by wordID
        unordered_map<int, vector<tuple<string,int,int>>> agg; // wordID -> list of (docID,freq,priority)
        ifstream hin(hitlistPath);
        string line;
        while (getline(hin, line)) {
            if (line.empty()) continue;
            // line format: wordID,docID,freq,priority,poslist
            // split first 5 commas safely (poslist may not contain commas)
            stringstream ss(line);
            string wid_s, docid, freq_s, prio_s, pos;
            if (!getline(ss, wid_s, ',')) continue;
            if (!getline(ss, docid, ',')) continue;
            if (!getline(ss, freq_s, ',')) continue;
            if (!getline(ss, prio_s, ',')) continue;
            if (!getline(ss, pos)) pos = "";
            int wid = stoi(wid_s);
            int freq = stoi(freq_s);
            int prio = stoi(prio_s);
            agg[wid].emplace_back(docid, freq, prio);
        }
        hin.close();

        // For each wordID in this hitlist (i.e. barrel), write one postings line
        ofstream pout2(outPostingsPath, ios::app);
        for (auto &kv : agg) {
            int wid = kv.first;
            auto &vec = kv.second;
            long long totalFreq = 0;
            string docIDsStr, freqPerDocStr, prioStr;
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i) {
                    docIDsStr += ";";
                    freqPerDocStr += ";";
                    prioStr += ";";
                }
                string docid; int freq, prio;
                tie(docid, freq, prio) = vec[i];
                docIDsStr += docid;
                freqPerDocStr += to_string(freq);
                prioStr += to_string(prio);
                totalFreq += freq;
            }
            pout2 << wid << "," << docIDsStr << "," << freqPerDocStr << "," << prioStr << "," << totalFreq << "\n";
        }
        pout2.close();
    }
}

// ---------------- MAIN ----------------
int main() {
    // create directories
    fs::create_directories("data/barrels");
    fs::create_directories("data/hitlists");

    // optional: load existing lexicon.csv from data/lexicon.csv
    string lexPath = "data/lexicon.csv";
    loadLexiconIfExists(lexPath);

    // paths - update if needed
    string metadataPath = "C:/Users/HC/Serach-Engine - Copy/cord-19_2020-05-26/2020-05-26/metadata.csv";
    string jsonFolder = "C:/Users/HC/Serach-Engine - Copy/cord-19_2020-05-26/2020-05-26/document_parses/document_parses/pmc_json/";

    if (!fs::exists(metadataPath)) {
        cerr << "metadata.csv not found at " << metadataPath << "\n";
        return 1;
    }

    // clear/create forward_index.csv (optional)
    ofstream fwdClear("data/forward_index.csv");
    fwdClear.close();

    // iterate metadata lines
    ifstream meta(metadataPath);
    string header;
    getline(meta, header); // skip header
    string raw;
    int docCount = 0;
    while (getline(meta, raw)) {
        if (raw.empty()) continue;
        vector<string> cols = splitCSVLine(raw);
        // metadata columns vary; but lexicon.cpp earlier used cord_id at col 0, title at col 2, abstract at col 8, authors maybe col 3 etc.
        // We'll attempt to extract common fields safely:
        string cord_id = (cols.size() > 0 ? cols[0] : "");
        string title   = (cols.size() > 2 ? cols[2] : "");
        string abstractText = "";
        // many metadata formats have abstract at different columns; try common indices
        if (cols.size() > 8) abstractText = cols[8];
        else if (cols.size() > 3) abstractText = cols[3]; // fallback
        string authors = (cols.size() > 3 ? cols[3] : "");

        if (cord_id.empty()) continue;

        // load JSON body
        string bodyText;
        string jsonPath = jsonFolder + cord_id + ".json";
        if (fs::exists(jsonPath)) {
            try {
                ifstream jf(jsonPath);
                json j; jf >> j;
                if (j.contains("body_text") && j["body_text"].is_array()) {
                    for (auto &p : j["body_text"]) {
                        if (p.contains("text") && p["text"].is_string()) {
                            bodyText += p["text"].get<string>() + " ";
                        }
                    }
                }
                jf.close();
            } catch (...) {
                // ignore parse errors for one document and continue
            }
        }

        string combinedTitle = title + " " + authors;
        processDocument(cord_id, combinedTitle, abstractText, bodyText);
        ++docCount;
        if ((docCount & 127) == 0) {
            cout << "Processed documents: " << docCount << "\r" << flush;
        }
    }
    meta.close();
    cout << "\nProcessed total documents: " << docCount << "\n";

    // save lexicon (exact format "word,wordID")
    saveLexicon(lexPath);
    cout << "Saved lexicon to " << lexPath << "\n";

    // Build postings.csv by reading hitlist files (per-barrel aggregation)
    string postingsOut = "data/postings.csv";
    buildPostingsFromHitlists("data/barrels", "data/hitlists", postingsOut);
    cout << "Postings written to " << postingsOut << "\n";

    cout << "Indexing finished!\n";
    return 0;
}
