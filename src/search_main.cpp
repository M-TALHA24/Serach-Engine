#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "tokenizer.h"
#include "lexicon.h"

int main() {
    Lexicon lex;
    lex.load("data/lexicon.csv");

    std::string query;
    std::cout << "Enter search query: ";
    getline(std::cin, query);

    auto tokens = tokenize(query);

    std::ifstream post("data/postings.csv");
    if(!post.is_open()){
        std::cerr << "postings.csv not found! Run the indexer first.\n";
        system("pause");
        return 1;
    }

    std::string line;
    bool found = false;

    while(getline(post,line)){
        for(auto &t:tokens){
            if(!lex.contains(t)) continue;
            int wid = lex.getWordID(t);
            if(line.rfind(std::to_string(wid)+",",0) == 0){ // starts_with for C++17/20
                std::cout << "MATCH: " << line << "\n";
                found = true;
            }
        }
    }

    if(!found) std::cout << "No results found.\n";

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}
