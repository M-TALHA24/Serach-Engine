#ifndef INVERTED_INDEX_H
#define INVERTED_INDEX_H

#include <vector>
#include <string>

void writeInverted(
    int wordID,
    const std::string &docID,
    int freq,
    int priority,
    const std::vector<int> &positions
);

void buildPostings();

#endif
