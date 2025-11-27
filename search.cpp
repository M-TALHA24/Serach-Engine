#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// ------------------ DATA STRUCTS ------------------
struct LexiconEntry {
    int wordID;
};

struct PostingEntry {
    vector<string> docIDs;
};

// ------------------ LOAD LEXICON ------------------
unordered_map<string, LexiconEntry> loadLexicon(const string& filename) {
    unordered_map<string, LexiconEntry> lexicon;
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Cannot open lexicon file: " << filename << endl;
        return lexicon;
    }

    string line;
    getline(fin, line); // skip header
    while (getline(fin, line)) {
        stringstream ss(line);
        string word;
        string wordIDStr;
        getline(ss, word, ',');
        getline(ss, wordIDStr, ',');
        LexiconEntry entry;
        entry.wordID = stoi(wordIDStr);
        lexicon[word] = entry;
    }
    fin.close();
    return lexicon;
}

// ------------------ LOAD POSTINGS ------------------
unordered_map<int, PostingEntry> loadPostings(const string& filename) {
    unordered_map<int, PostingEntry> postings;
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Cannot open postings file: " << filename << endl;
        return postings;
    }

    string line;
    getline(fin, line); // skip header
    while (getline(fin, line)) {
        stringstream ss(line);
        string wordIDStr, docIDsStr, freqStr, priorityStr, totalFreq;
        getline(ss, wordIDStr, ',');
        getline(ss, docIDsStr, ',');
        getline(ss, freqStr, ',');
        getline(ss, priorityStr, ',');
        getline(ss, totalFreq, ',');

        PostingEntry entry;
        stringstream docs(docIDsStr);
        string doc;
        while (getline(docs, doc, ';')) {
            entry.docIDs.push_back(doc);
        }

        postings[stoi(wordIDStr)] = entry;
    }
    fin.close();
    return postings;
}

// ------------------ LOAD CORD_PROCESSED URLs ------------------
unordered_map<string, string> loadDocURLs(const string& filename) {
    unordered_map<string, string> docURL;
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Cannot open processed file: " << filename << endl;
        return docURL;
    }

    string line;
    getline(fin, line); // skip header
    while (getline(fin, line)) {
        stringstream ss(line);
        string cord_id, url;
        getline(ss, cord_id, ',');
        getline(ss, url, ','); // first field is cord_id, second is url
        docURL[cord_id] = url;
    }
    fin.close();
    return docURL;
}

// ------------------ MAIN SEARCH FUNCTION ------------------
int main() {
    string word;
    cout << "Enter word to search: ";
    cin >> word;

    // Load files
    auto lexicon = loadLexicon("lexicon.csv");
    auto postings = loadPostings("postings.csv");
    auto docURLs = loadDocURLs("cord_processed.csv");

    auto it = lexicon.find(word);
    if (it == lexicon.end()) {
        cout << "Word not found in lexicon!" << endl;
        return 0;
    }

    int wordID = it->second.wordID;
    auto postIt = postings.find(wordID);
    if (postIt == postings.end()) {
        cout << "No postings found for this word!" << endl;
        return 0;
    }

    cout << "Documents containing the word '" << word << "':" << endl;
    for (const auto& docID : postIt->second.docIDs) {
        if (docURLs.find(docID) != docURLs.end()) {
            cout << docID << " -> " << docURLs[docID] << endl;
        } else {
            cout << docID << " -> URL not found" << endl;
        }
    }

    return 0;
}
