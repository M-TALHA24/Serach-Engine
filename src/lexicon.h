#ifndef LEXICON_H
#define LEXICON_H
#include "trie.h"
#include <unordered_map>
#include <string>

class Lexicon
{
private:
    std::unordered_map<std::string, int> wordToID;
    int nextID = 0;
    Trie trie; // ← ADD

public:
    int getExistingWordID(const std::string &word) const;

    void load(const std::string &path);
    void save(const std::string &path);
    int getWordID(const std::string &word);
    bool contains(const std::string &word) const;

    std::vector<std::string> autocomplete(
        const std::string &prefix, int k = 10) const; // ← ADD
};

#endif
