#ifndef FORWARD_INDEX_H
#define FORWARD_INDEX_H

#include <string>
#include <unordered_set>

void writeForwardIndex(
    const std::string &docID,
    const std::unordered_set<int> &wordSet);

#endif
