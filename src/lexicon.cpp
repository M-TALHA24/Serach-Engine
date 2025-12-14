#include "lexicon.h"
#include <fstream>
#include <sstream>

void Lexicon::load(const std::string &path) {
    std::ifstream in(path);
    if (!in.is_open()) return;
    std::string line;
    getline(in, line); // header
    while (getline(in, line)) {
        std::stringstream ss(line);
        std::string w; int id;
        getline(ss, w, ',');
        ss >> id;
        wordToID[w] = id;
        nextID = std::max(nextID, id + 1);
    }
}

void Lexicon::save(const std::string &path) {
    std::ofstream out(path);
    out << "word,wordID\n";
    for (auto &p : wordToID)
        out << p.first << "," << p.second << "\n";
}

int Lexicon::getWordID(const std::string &word) {
    auto it = wordToID.find(word);
    if (it != wordToID.end()) return it->second;
    int id = nextID++;
    wordToID[word] = id;
    return id;
}

bool Lexicon::contains(const std::string &word) const {
    return wordToID.count(word);
}
