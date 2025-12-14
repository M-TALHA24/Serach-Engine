#include "forward_index.h"
#include <fstream>

void writeForwardIndex(
    const std::string &docID,
    const std::unordered_map<int,int> &freqMap
) {
    std::ofstream out("data/forward_index.csv", std::ios::app);
    out << docID << ",";
    bool first = true;
    for (auto &p : freqMap) {
        if (!first) out << ";";
        first = false;
        out << p.first << ":" << p.second;
    }
    out << "\n";
}
