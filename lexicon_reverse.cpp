#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// ---------------- LEXICON ENTRY STRUCT -----------------
struct LexiconEntry {
    int wordID;
    vector<string> docIDs;
    LexiconEntry() : wordID(0) {}
};

// ---------------- SPLIT FUNCTION -----------------
vector<string> splitWords(const string& text) {
    vector<string> words;
    stringstream ss(text);
    string word;
    while (ss >> word) words.push_back(word);
    return words;
}

// ---------------- DISPLAY ELAPSED TIME -----------------
void displayElapsedTime(system_clock::time_point startTime) {
    auto now = system_clock::now();
    auto elapsed = duration_cast<seconds>(now - startTime).count();
    int hrs = elapsed / 3600;
    int mins = (elapsed % 3600) / 60;
    int secs = elapsed % 60;
    cout << "\rElapsed time: "
         << setw(2) << setfill('0') << hrs << ":"
         << setw(2) << setfill('0') << mins << ":"
         << setw(2) << setfill('0') << secs
         << flush;
}

int main() {
    string filename = "cord_processed.csv"; // Input CSV
    unordered_map<string, LexiconEntry> lexicon;
    unordered_set<string> processed_docs;
    int wordID_counter = 0;

    // ---------------- Load existing lexicon -----------------
    ifstream lexFileIn("lexicon.csv");
    if (lexFileIn.is_open()) {
        string line;
        getline(lexFileIn, line); // skip header
        while (getline(lexFileIn, line)) {
            stringstream ss(line);
            string word, wordIDStr;
            getline(ss, word, ',');
            getline(ss, wordIDStr, ',');

            LexiconEntry entry;
            entry.wordID = stoi(wordIDStr);
            lexicon[word] = entry;
            wordID_counter = max(wordID_counter, entry.wordID + 1);
        }
        lexFileIn.close();
        cout << "Loaded existing lexicon. Total words: " << lexicon.size() << endl;
    }

    // ---------------- Process CSV -----------------
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Cannot open file: " << filename << endl;
        return 1;
    }

    string line;
    getline(file, line); // skip header
    auto startTime = system_clock::now();
    int processed_lines = 0;

    while (getline(file, line)) {
        stringstream ss(line);
        string cord_id, url, authors, title, abstract, body_text, journal;
        getline(ss, cord_id, ',');
        getline(ss, url, ',');
        getline(ss, authors, ',');
        getline(ss, title, ',');
        getline(ss, abstract, ',');
        getline(ss, body_text, ',');
        getline(ss, journal, ',');

        if (processed_docs.find(cord_id) != processed_docs.end()) continue; // skip existing

        vector<string> all_words;
        auto addWords = [&](const string& text){ 
            vector<string> words = splitWords(text);
            all_words.insert(all_words.end(), words.begin(), words.end());
        };
        addWords(title + " " + authors);
        addWords(abstract);
        addWords(body_text);

        unordered_set<string> unique_words(all_words.begin(), all_words.end());

        for (const string& word : unique_words) {
            auto it = lexicon.find(word);
            if (it == lexicon.end()) {
                LexiconEntry entry;
                entry.wordID = wordID_counter++;
                entry.docIDs.push_back(cord_id);
                lexicon[word] = entry;
            } else {
                LexiconEntry &entry = it->second;
                entry.docIDs.push_back(cord_id);
            }
        }

        processed_docs.insert(cord_id);
        processed_lines++;
        if (processed_lines % 10 == 0) displayElapsedTime(startTime);
    }

    file.close();

    cout << "\nLexicon built! Total unique words: " << lexicon.size() << endl;

    // ---------------- Save lexicon.csv -----------------
    ofstream lexFileOut("lexicon.csv");
    lexFileOut << "word,wordID\n";
    for (auto& [word, entry] : lexicon) {
        lexFileOut << word << "," << entry.wordID << "\n";
    }
    lexFileOut.close();
    cout << "Lexicon saved to: lexicon.csv" << endl;

    // ---------------- Save postings.csv -----------------
    ofstream postFileOut("postings.csv");
    postFileOut << "wordID,docIDs\n";
    for (auto& [word, entry] : lexicon) {
        postFileOut << entry.wordID << ",";
        for (size_t i=0; i<entry.docIDs.size(); ++i) {
            postFileOut << entry.docIDs[i];
            if (i != entry.docIDs.size()-1) postFileOut << ";";
        }
        postFileOut << "\n";
    }
    postFileOut.close();
    cout << "Postings saved to: postings.csv" << endl;

    return 0;
}
