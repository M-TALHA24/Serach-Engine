#include <bits/stdc++.h>
#include <filesystem>
#include <nlohmann/json.hpp> // JSON library
using json = nlohmann::json;

using namespace std;
namespace fs = std::filesystem;

const int BARREL_SIZE = 5000;

struct WordInfo {
    vector<int> positions;
    int priority; // 1 = title, 2 = abstract, 3 = body
};

unordered_map<string, int> lexicon;
int nextWordID = 0;

string clean(string s) {
    for (auto &c : s) {
        if (!isalpha(c)) c = ' ';
        else c = tolower(c);
    }
    return s;
}

vector<string> tokenize(const string &text) {
    string cleaned = clean(text);
    stringstream ss(cleaned);
    vector<string> words;
    string w;
    while (ss >> w) words.push_back(w);
    return words;
}

void loadLexicon(const string &path) {
    ifstream file(path);
    if (!file.is_open()) return;

    string line;
    getline(file, line); // header
    while (getline(file, line)) {
        stringstream ss(line);
        string word; int id;
        getline(ss, word, ',');
        ss >> id;
        lexicon[word] = id;
        nextWordID = max(nextWordID, id + 1);
    }
}

int getWordID(const string &w) {
    auto it = lexicon.find(w);
    if (it != lexicon.end()) return it->second;
    int id = nextWordID++;
    lexicon[w] = id;
    return id;
}

void saveLexicon(const string &path) {
    ofstream out(path);
    out << "word,wordID\n";
    for (auto &p : lexicon) {
        out << p.first << "," << p.second << "\n";
    }
}

int getBarrelID(int wordID) {
    return wordID / BARREL_SIZE;
}

// append line to file
void appendToFile(const string &path, const string &line) {
    ofstream out(path, ios::app);
    out << line << "\n";
}

void processDocument(
    const string &docID,
    const string &title,
    const string &abstractText,
    const string &body
) {
    unordered_map<int, WordInfo> wordData;

    auto addSection = [&](const string &text, int priority) {
        vector<string> words = tokenize(text);
        for (int pos = 0; pos < words.size(); pos++) {
            string w = words[pos];
            if (w.empty()) continue;

            int wordID = getWordID(w);
            wordData[wordID].positions.push_back(pos);
            wordData[wordID].priority = priority;
        }
    };

    addSection(title, 1);
    addSection(abstractText, 2);
    addSection(body, 3);

    // Write forward index
    {
        ofstream fwd("data/forward_index.csv", ios::app);
        fwd << docID << ",";
        bool first = true;
        for (auto &p : wordData) {
            if (!first) fwd << ";";
            first = false;
            fwd << p.first << ":" << p.second.positions.size();
        }
        fwd << "\n";
    }

    // Write barrels + hitlists
    for (auto &p : wordData) {
        int wordID = p.first;
        int freq = p.second.positions.size();
        int priority = p.second.priority;

        int barrelID = getBarrelID(wordID);

        string barrelPath = "data/barrels/barrel_" + to_string(barrelID) + ".csv";
        appendToFile(barrelPath,
                     to_string(wordID) + "," +
                     docID + "," +
                     to_string(freq));

        string hitlistPath = "data/hitlists/hitlist_" + to_string(barrelID) + ".csv";

        string posList;
        for (int i = 0; i < p.second.positions.size(); i++) {
            if (i) posList += "|";
            posList += to_string(p.second.positions[i]);
        }

        appendToFile(hitlistPath,
                     to_string(wordID) + "," +
                     docID + "," +
                     to_string(freq) + "," +
                     to_string(priority) + "," +
                     posList);
    }
}

int main() {
    // Create folders
    fs::create_directories("data/barrels");
    fs::create_directories("data/hitlists");

    // Load existing lexicon if any
    loadLexicon("data/lexicon.csv");

    string metadataPath = "C:/Users/HC/Serach-Engine - Copy/cord-19_2020-05-26/2020-05-26/metadata.csv";
    string jsonFolder = "C:/Users/HC/Serach-Engine - Copy/cord-19_2020-05-26/2020-05-26/document_parses/document_parses/pmc_json/";

    ifstream meta(metadataPath);
    if (!meta.is_open()) {
        cerr << "Cannot open metadata file!" << endl;
        return 1;
    }

    string line;
    getline(meta, line); // skip header

    while (getline(meta, line)) {
        stringstream ss(line);
        string cord_id, title, abstractText, authors, journal, url;
        getline(ss, cord_id, ',');
        getline(ss, title, ',');
        getline(ss, abstractText, ',');
        getline(ss, authors, ',');
        getline(ss, journal, ',');
        getline(ss, url, ',');

        // Read JSON file for body
        string jsonPath = jsonFolder + cord_id + ".json";
        string bodyText;

        ifstream jf(jsonPath);
        if (jf.is_open()) {
            json j;
            jf >> j;
            for (auto &p : j["body_text"]) {
                bodyText += p["text"].get<string>() + " ";
            }
            jf.close();
        }

        // Combine title + abstract + authors + body
        string combinedTitle = title + " " + authors;
        processDocument(cord_id, combinedTitle, abstractText, bodyText);
    }

    meta.close();

    saveLexicon("data/lexicon.csv");

    cout << "Indexing finished!" << endl;

    return 0;
}
