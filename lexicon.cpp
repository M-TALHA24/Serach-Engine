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
    vector<int> freqPerDoc;
    vector<int> priority;
    int totalFrequency;
    LexiconEntry() : wordID(0), totalFrequency(0) {}
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

        vector<pair<vector<string>, int>> sections = {
            {splitWords(title + " " + authors), 1},
            {splitWords(abstract), 2},
            {splitWords(body_text), 3}
        };

        unordered_map<string, pair<int,int>> word_count;
        for (auto& sec : sections) {
            vector<string>& words = sec.first;
            int prio = sec.second;
            for (string& w : words) {
                auto it = word_count.find(w);
                if (it != word_count.end()) {
                    it->second.first += 1;
                    it->second.second = min(it->second.second, prio);
                } else {
                    word_count[w] = {1, prio};
                }
            }
        }

        for (auto& [word, info] : word_count) {
            int count = info.first;
            int prio = info.second;

            auto it = lexicon.find(word);
            if (it == lexicon.end()) {
                LexiconEntry entry;
                entry.wordID = wordID_counter++;
                entry.docIDs.push_back(cord_id);
                entry.freqPerDoc.push_back(count);
                entry.priority.push_back(prio);
                entry.totalFrequency = count;
                lexicon[word] = entry;
            } else {
                LexiconEntry &entry = it->second;
                entry.docIDs.push_back(cord_id);
                entry.freqPerDoc.push_back(count);
                entry.priority.push_back(prio);
                entry.totalFrequency += count;
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
    postFileOut << "wordID,docIDs,freqPerDoc,priority,totalFrequency\n";
    for (auto& [word, entry] : lexicon) {
        postFileOut << entry.wordID << ",";
        for (size_t i=0; i<entry.docIDs.size(); ++i) {
            postFileOut << entry.docIDs[i];
            if (i != entry.docIDs.size()-1) postFileOut << ";";
        }
        postFileOut << ",";
        for (size_t i=0; i<entry.freqPerDoc.size(); ++i) {
            postFileOut << entry.freqPerDoc[i];
            if (i != entry.freqPerDoc.size()-1) postFileOut << ";";
        }
        postFileOut << ",";
        for (size_t i=0; i<entry.priority.size(); ++i) {
            postFileOut << entry.priority[i];
            if (i != entry.priority.size()-1) postFileOut << ";";
        }
        postFileOut << "," << entry.totalFrequency << "\n";
    } 
    postFileOut.close();
    cout << "Postings saved to: postings.csv" << endl;
    cout << endl;
    return 0;
}
