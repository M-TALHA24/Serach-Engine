#pragma once
#include <unordered_map>
#include <vector>
#include <string>

struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    bool isEnd = false;
};

class Trie {
private:
    TrieNode* root;
    void dfs(TrieNode* node, std::string prefix,
             std::vector<std::string>& results) const;

public:
    Trie();
    void insert(const std::string& word);
    std::vector<std::string> autocomplete(
        const std::string& prefix, int limit = 10) const;
};