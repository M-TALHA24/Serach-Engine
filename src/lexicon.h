#ifndef LEXICON_H
#define LEXICON_H

#include <unordered_map>
#include <string>

class Lexicon {
public:
    void load(const std::string &path);
    void save(const std::string &path);
    int getWordID(const std::string &word);
    bool contains(const std::string &word) const;

private:
    std::unordered_map<std::string,int> wordToID;
    int nextID = 0;
};

#endif
