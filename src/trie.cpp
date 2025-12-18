#include "trie.h"

Trie::Trie() {
    root = new TrieNode();
}

void Trie::insert(const std::string& word) {
    TrieNode* node = root;
    for (char c : word) {
        if (!node->children[c])
            node->children[c] = new TrieNode();
        node = node->children[c];
    }
    node->isEnd = true;
}

void Trie::dfs(TrieNode* node, std::string prefix,
               std::vector<std::string>& results) const {
    if (node->isEnd)
        results.push_back(prefix);

    for (auto& p : node->children) {
        dfs(p.second, prefix + p.first, results);
    }
}

std::vector<std::string> Trie::autocomplete(
        const std::string& prefix, int limit) const {

    TrieNode* node = root;
    for (char c : prefix) {
        if (!node->children.count(c))
            return {};
        node = node->children[c];
    }

    std::vector<std::string> results;
    dfs(node, prefix, results);
    if ((int)results.size() > limit)
        results.resize(limit);
    return results;
}