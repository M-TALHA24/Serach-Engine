#include "forward_index.h"
#include <fstream>

void writeForwardIndex(
    const std::string &docID,
    const std::unordered_set<int> &wordSet)
{
    std::ofstream out("data/forward_index.csv", std::ios::app);
    out << docID << ",";

    bool first = true;
    for (int wordID : wordSet)
    {
        if (!first)
            out << ";";
        first = false;
        out << wordID;
    }

    out << "\n";
}
