#include "tokenizer.h"
#include "lexicon.h"
#include "forward_index.h"
#include "inverted_index.h"
#include "text_normalizer.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

int main()
{
    Lexicon lex;
    lex.load("data/lexicon.csv");

    std::string metadataPath =
        "C:/Users/HC/Serach-Engine - Copy/cord-19_2020-05-26/2020-05-26/metadata.csv";
    std::string jsonFolder =
        "C:/Users/HC/Serach-Engine - Copy/cord-19_2020-05-26/2020-05-26/document_parses/document_parses/pmc_json/";

    std::ifstream meta(metadataPath);
    if (!meta.is_open())
    {
        std::cerr << "Cannot open metadata.csv at " << metadataPath << "\n";
        return 1;
    }

    std::string header;
    getline(meta, header); // skip header

    std::string line;
    int docCount = 0;

    while (getline(meta, line))
    {
        if (line.empty())
            continue;

        std::stringstream ss(line);
        std::vector<std::string> cols;
        std::string col;
        while (getline(ss, col, ','))
            cols.push_back(col);

        std::string docID = (cols.size() > 0) ? cols[0] : "";
        std::string title = (cols.size() > 2) ? cols[2] : "";
        std::string abstractText = (cols.size() > 8) ? cols[8] : "";

        if (docID.empty())
            continue;

        // -------- READ JSON BODY --------
        std::string body;
        std::string jsonPath = jsonFolder + docID + ".json";
        if (fs::exists(jsonPath))
        {
            std::ifstream jf(jsonPath);
            if (jf.is_open())
            {
                std::string jline;
                while (getline(jf, jline))
                    body += jline + " ";
                jf.close();
            }
        }

        // -------- DATA STRUCTURES --------
        std::unordered_map<int, int> freqMap;             // for inverted index
        std::unordered_map<int, std::vector<int>> posMap; // positions
        std::unordered_set<int> wordSet;                  // for forward index

        // -------- PROCESS TEXT --------
        auto processText = [&](const std::string &text, int priority)
        {
            std::string normalized = TextNormalizer::normalize(text);
            auto tokens = tokenize(normalized);

            for (size_t i = 0; i < tokens.size(); ++i)
            {
                int wid = lex.getWordID(tokens[i]);

                freqMap[wid]++;
                posMap[wid].push_back(static_cast<int>(i));
                wordSet.insert(wid);

                writeInverted(
                    wid,
                    docID,
                    freqMap[wid], // term frequency (needed here)
                    priority,
                    posMap[wid]);
            }
        };

        // -------- INDEX SECTIONS --------
        processText(title, 1);
        processText(abstractText, 2);
        processText(body, 3);

        // -------- FORWARD INDEX (NO FREQUENCY) --------
        writeForwardIndex(docID, wordSet);

        ++docCount;
        if ((docCount & 127) == 0)
            std::cout << "Processed docs: " << docCount << "\r";
    }

    meta.close();

    lex.save("data/lexicon.csv");

    // -------- BUILD POSTINGS --------
    buildPostings();

    std::cout << "\nIndexing finished! Total docs: " << docCount << "\n";
    return 0;
}
