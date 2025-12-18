#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "tokenizer.h"
#include "lexicon.h"

using namespace std;

struct Result
{
    string docID;
    int score;
};

int main()
{
    // ---------------- LOAD LEXICON ----------------
    Lexicon lex;
    lex.load("data/lexicon.csv");

    // ---------------- LOAD POSTINGS ----------------
    ifstream post("data/postings.csv");
    if (!post.is_open())
    {
        cout << "ERROR: data/postings.csv not found\n";
        return 1;
    }

    // ---------------- LOAD DOCID → URL ----------------
    unordered_map<string, string> docURL;
    {
        ifstream urlFile("data/doc_urls.csv");
        if (!urlFile.is_open())
        {
            cout << "ERROR: data/doc_urls.csv not found\n";
            return 1;
        }

        string line;
        getline(urlFile, line); // skip header

        while (getline(urlFile, line))
        {
            string id, url;
            stringstream ss(line);
            getline(ss, id, ',');
            getline(ss, url);
            docURL[id] = url;
        }
    }

    // ---------------- INPUT QUERY ----------------
    cout << "Enter search query: ";
    string query;
    getline(cin, query);

    if (query.empty())
    {
        cout << "Empty query\n";
        return 0;
    }

    // ---------------- LOGIC DETECTION ----------------
    bool isAND = (query.find("AND") != string::npos);
    bool isOR = (query.find("OR") != string::npos);

    vector<string> terms = tokenize(query);
    if (terms.empty())
    {
        cout << "No valid terms\n";
        return 0;
    }

    unordered_map<string, int> docScores;
    unordered_map<string, int> docHitCount;

    // ---------------- PROCESS POSTINGS ----------------
    string line;
    getline(post, line); // skip header

    while (getline(post, line))
    {
        stringstream ss(line);

        int wid, totalFreq;
        string docIDs, freqs, prios;

        ss >> wid;
        ss.ignore(); // comma
        getline(ss, docIDs, ',');
        getline(ss, freqs, ',');
        getline(ss, prios, ',');
        ss >> totalFreq;

        for (auto &t : terms)
        {
            if (!lex.contains(t))
                continue;

            int qid = lex.getWordID(t);
            if (qid != wid)
                continue;

            stringstream dss(docIDs);
            stringstream fss(freqs);

            string d, f;
            while (getline(dss, d, ';') && getline(fss, f, ';'))
            {
                int freq = stoi(f);
                docScores[d] += freq;
                docHitCount[d]++;
            }
        }
    }

    // ---------------- FILTER AND RANK ----------------
    vector<Result> results;
    for (auto &p : docScores)
    {
        if (isAND && docHitCount[p.first] < (int)terms.size())
            continue;

        results.push_back({p.first, p.second});
    }

    sort(results.begin(), results.end(),
         [](const Result &a, const Result &b)
         {
             return a.score > b.score;
         });

    // ---------------- OUTPUT ----------------
    if (results.empty())
    {
        cout << "No results found\n";
        return 0;
    }

    cout << "\nSearch Results (Ranked):\n";
    for (auto &r : results)
    {
        cout << "DocID: " << r.docID
             << " | Score: " << r.score;

        if (docURL.count(r.docID))
            cout << " | URL: " << docURL[r.docID];

        cout << "\n";
    }

    return 0;
}
