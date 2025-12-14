#include "inverted_index.h"
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <tuple>

namespace fs = std::filesystem;
const int BARREL_SIZE = 1000;

static int getBarrelID(int wordID) {
    return wordID / BARREL_SIZE;
}

void writeInverted(
    int wordID,
    const std::string &docID,
    int freq,
    int priority,
    const std::vector<int> &positions
) {
    int bid = getBarrelID(wordID);
    std::string barrel = "data/barrels/barrel_" + std::to_string(bid) + ".csv";
    std::string hit = "data/hitlists/hitlist_" + std::to_string(bid) + ".csv";

    std::ofstream b(barrel, std::ios::app);
    b << wordID << "," << docID << "," << freq << "\n";

    std::ofstream h(hit, std::ios::app);
    h << wordID << "," << docID << "," << freq << "," << priority << ",";
    for (size_t i = 0; i < positions.size(); ++i) {
        if (i) h << "|";
        h << positions[i];
    }
    h << "\n";
}

void buildPostings() {
    std::ofstream out("data/postings.csv");
    out << "wordID,docIDs,freqs,priorities,totalFreq\n";

    for (auto &e : fs::directory_iterator("data/hitlists")) {
        std::unordered_map<int,std::vector<std::tuple<std::string,int,int>>> agg;
        std::ifstream in(e.path());
        std::string line;
        while (getline(in, line)) {
            std::stringstream ss(line);
            int wid,freq,prio;
            std::string doc;
            ss >> wid; ss.ignore();
            getline(ss, doc, ',');
            ss >> freq; ss.ignore();
            ss >> prio;
            agg[wid].push_back({doc,freq,prio});
        }

        for (auto &p : agg) {
            int total = 0;
            std::string docs,freqs,prios;
            for (size_t i=0;i<p.second.size();++i) {
                auto &[d,f,r]=p.second[i];
                if(i){docs+=";";freqs+=";";prios+=";";}
                docs+=d; freqs+=std::to_string(f); prios+=std::to_string(r);
                total+=f;
            }
            out<<p.first<<","<<docs<<","<<freqs<<","<<prios<<","<<total<<"\n";
        }
    }
}
