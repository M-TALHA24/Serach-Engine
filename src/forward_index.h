#ifndef FORWARD_INDEX_H
#define FORWARD_INDEX_H

#include <string>
#include <unordered_map>

void writeForwardIndex(
    const std::string &docID,
    const std::unordered_map<int,int> &freqMap
);

#endif
