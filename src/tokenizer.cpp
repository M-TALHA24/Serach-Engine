#include "tokenizer.h"
#include <sstream>

std::vector<std::string> tokenize(const std::string &text)
{
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string word;
    while (ss >> word)
        tokens.push_back(word);
    return tokens;
}
