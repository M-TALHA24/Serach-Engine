#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <unordered_set>
using namespace std;

// ---------------- SPLIT FUNCTION ----------------
vector<string> splitWords(const string& text) {
    vector<string> words;
    stringstream ss(text);
    string word;
    while (ss >> word) words.push_back(word);
    return words;
}

// ---------------- STRUCTS ----------------
struct WordInfo {
    int wordID;
    int freq;
    int priority;
};

int main() {
    string inputCSV = "cord_processed.csv";
    string lexiconCSV = "lexicon.csv";          // Existing lexicon (from inverted index)
    string outputForward = "forward_index.csv"; // Output forward index

    // -------- LOAD EXISTING LEXICON --------
    unordered_map<string, int> lexicon; // word -> wordID
    ifstream lexIn(lexiconCSV);
    if (!lexIn.is_open()) {
        cerr << "Cannot open lexicon file: " << lexiconCSV << endl;
        return 1;
    }

    string line;
    getline(lexIn, line); // skip header
    while (getline(lexIn, line)) {
        stringstream ss(line);
        string word;
        int wordID;
        getline(ss, word, ',');
        ss >> wordID;
        lexicon[word] = wordID;
    }
    lexIn.close();
    cout << "Lexicon loaded. Total words: " << lexicon.size() << endl;

    // -------- PROCESS DOCUMENTS --------
    unordered_map<string, vector<WordInfo>> forwardIndex;
    ifstream fin(inputCSV);
    if (!fin.is_open()) {
        cerr << "Cannot open file: " << inputCSV << endl;
        return 1;
    }

    getline(fin, line); // skip header
    while (getline(fin, line)) {
        stringstream ss(line);
        string docID, url, authors, title, abstract, body_text, journal;
        getline(ss, docID, ',');
        getline(ss, url, ',');
        getline(ss, authors, ',');
        getline(ss, title, ',');
        getline(ss, abstract, ',');
        getline(ss, body_text, ',');
        getline(ss, journal, ',');

        if (docID.empty()) continue;

        // -------- Split sections with priority --------
        vector<pair<vector<string>, int>> sections = {
            {splitWords(title + " " + authors), 1},
            {splitWords(abstract), 2},
            {splitWords(body_text), 3}
        };

        unordered_map<string, pair<int,int>> localCount; // word -> (freq, priority)
        for (auto& sec : sections) {
            auto& words = sec.first;
            int prio = sec.second;
            for (auto& w : words) {
                if (!lexicon.count(w)) continue; // ignore words not in lexicon
                if (localCount.count(w)) {
                    localCount[w].first += 1;
                    localCount[w].second = min(localCount[w].second, prio);
                } else {
                    localCount[w] = {1, prio};
                }
            }
        }

        // Build forward index for this document
        vector<WordInfo> docWords;
        for (auto& [word, info] : localCount) {
            docWords.push_back({lexicon[word], info.first, info.second});
        }
        forwardIndex[docID] = docWords;
    }
    fin.close();

    // -------- SAVE FORWARD INDEX --------
    ofstream fwdOut(outputForward);
    fwdOut << "docID,wordIDs,freqs,priorities\n";
    for (auto& [docID, words] : forwardIndex) {
        fwdOut << docID << ",";
        for (size_t i = 0; i < words.size(); i++) {
            fwdOut << words[i].wordID;
            if (i != words.size()-1) fwdOut << ";";
        }
        fwdOut << ",";
        for (size_t i = 0; i < words.size(); i++) {
            fwdOut << words[i].freq;
            if (i != words.size()-1) fwdOut << ";";
        }
        fwdOut << ",";
        for (size_t i = 0; i < words.size(); i++) {
            fwdOut << words[i].priority;
            if (i != words.size()-1) fwdOut << ";";
        }
        fwdOut << "\n";
    }
    fwdOut.close();
    cout << "Forward index saved to " << outputForward << endl;

    return 0;
}