#include "tokenizer.h"
#include <sstream>
#include <cctype>

std::string clean(const std::string &s) {
    std::string res = s;
    for (char &c : res) {
        if (!isalpha((unsigned char)c)) c = ' ';
        else c = tolower((unsigned char)c);
    }
    return res;
}

std::vector<std::string> tokenize(const std::string &text) {
    std::vector<std::string> words;
    std::stringstream ss(clean(text));
    std::string w;
    while (ss >> w) words.push_back(w);
    return words;
}
