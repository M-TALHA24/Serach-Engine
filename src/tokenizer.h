#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>

std::string clean(const std::string &s);
std::vector<std::string> tokenize(const std::string &text);

#endif
