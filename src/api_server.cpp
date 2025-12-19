// ============================================
// SEARCH API SERVER
// A simple HTTP server that handles search requests
// Returns JSON results to the React frontend
// Features: BM25 semantic ranking, autocomplete
// ============================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <chrono>

// Windows socket headers
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

using namespace std;

// ============================================
// DATA STRUCTURES
// ============================================

// Structure to hold document metadata
struct Document
{
    string docId;
    string title;
    string authors;
    string abstract;
    string url;
};

// Structure for search results with ranking score
struct SearchResult
{
    string docId;
    string title;
    string authors;
    string abstract;
    string url;
    double score;
};

// ============================================
// TRIE FOR AUTOCOMPLETE
// ============================================
struct TrieNode
{
    unordered_map<char, TrieNode *> children;
    bool isEnd = false;
};

class Trie
{
private:
    TrieNode *root;

    void dfs(TrieNode *node, string prefix, vector<string> &results, int limit) const
    {
        if ((int)results.size() >= limit)
            return;
        if (node->isEnd)
            results.push_back(prefix);
        for (auto &p : node->children)
        {
            dfs(p.second, prefix + p.first, results, limit);
        }
    }

public:
    Trie() { root = new TrieNode(); }

    void insert(const string &word)
    {
        TrieNode *node = root;
        for (char c : word)
        {
            if (!node->children[c])
                node->children[c] = new TrieNode();
            node = node->children[c];
        }
        node->isEnd = true;
    }

    vector<string> autocomplete(const string &prefix, int limit = 10) const
    {
        TrieNode *node = root;
        for (char c : prefix)
        {
            if (!node->children.count(c))
                return {};
            node = node->children[c];
        }
        vector<string> results;
        dfs(node, prefix, results, limit);
        return results;
    }
};

// ============================================
// GLOBAL DATA (loaded at startup)
// ============================================
unordered_map<string, int> lexicon;                     // word -> wordID
unordered_map<string, Document> documents;              // docId -> document info
unordered_map<int, vector<pair<string, int>>> postings; // wordID -> [(docId, freq)]
unordered_map<string, string> docUrls;                  // docId -> URL
Trie autocompleteTrie;                                  // Trie for word suggestions

// BM25 Parameters and Statistics
unordered_map<string, int> docLengths; // docId -> document length (total terms)
unordered_map<int, int> docFrequency;  // wordID -> number of documents containing this word
double avgDocLength = 0.0;             // Average document length
int totalDocuments = 0;                // Total number of documents

// BM25 tuning parameters
const double k1 = 1.5; // Term frequency saturation parameter
const double b = 0.75; // Document length normalization parameter

// ============================================
// HELPER FUNCTIONS
// ============================================

// Clean and tokenize text (same as tokenizer.cpp)
string cleanText(const string &text)
{
    string result = text;
    for (char &c : result)
    {
        if (!isalpha((unsigned char)c))
            c = ' ';
        else
            c = tolower((unsigned char)c);
    }
    return result;
}

vector<string> tokenize(const string &text)
{
    vector<string> words;
    stringstream ss(cleanText(text));
    string word;
    while (ss >> word)
    {
        words.push_back(word);
    }
    return words;
}

// URL decode function for handling spaces and special chars
string urlDecode(const string &str)
{
    string result;
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] == '%' && i + 2 < str.length())
        {
            int value;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            result += static_cast<char>(value);
            i += 2;
        }
        else if (str[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

// Extract query parameter from URL
string getQueryParam(const string &request)
{
    size_t qPos = request.find("?q=");
    if (qPos == string::npos)
        return "";

    size_t start = qPos + 3;
    size_t end = request.find(" ", start);
    if (end == string::npos)
        end = request.find("&", start);
    if (end == string::npos)
        end = request.length();

    return urlDecode(request.substr(start, end - start));
}

// ============================================
// DATA LOADING FUNCTIONS
// ============================================

void loadLexicon(const string &path)
{
    ifstream file(path);
    if (!file.is_open())
    {
        cerr << "Warning: Could not open lexicon at " << path << endl;
        return;
    }

    string line;
    getline(file, line); // Skip header

    while (getline(file, line))
    {
        stringstream ss(line);
        string word;
        int id;
        getline(ss, word, ',');
        ss >> id;
        lexicon[word] = id;
        autocompleteTrie.insert(word); // Build trie for autocomplete
    }

    cout << "Loaded " << lexicon.size() << " words from lexicon" << endl;
}

void loadPostings(const string &path)
{
    ifstream file(path);
    if (!file.is_open())
    {
        cerr << "Warning: Could not open postings at " << path << endl;
        return;
    }

    string line;
    getline(file, line); // Skip header

    while (getline(file, line))
    {
        stringstream ss(line);
        int wordId;
        string docIds, freqs;

        ss >> wordId;
        ss.ignore();
        getline(ss, docIds, ',');
        getline(ss, freqs, ',');

        // Parse document IDs and frequencies
        vector<string> docs;
        vector<int> frequencies;

        stringstream docSS(docIds);
        string doc;
        while (getline(docSS, doc, ';'))
        {
            docs.push_back(doc);
        }

        stringstream freqSS(freqs);
        string freq;
        while (getline(freqSS, freq, ';'))
        {
            frequencies.push_back(stoi(freq));
        }

        // Track document frequency for IDF calculation
        docFrequency[wordId] = docs.size();

        for (size_t i = 0; i < docs.size() && i < frequencies.size(); i++)
        {
            postings[wordId].push_back({docs[i], frequencies[i]});
            // Track document lengths for BM25 normalization
            docLengths[docs[i]] += frequencies[i];
        }
    }

    // Calculate average document length
    long long totalLength = 0;
    for (const auto &dl : docLengths)
    {
        totalLength += dl.second;
    }
    totalDocuments = docLengths.size();
    avgDocLength = totalDocuments > 0 ? (double)totalLength / totalDocuments : 1.0;

    cout << "Loaded postings for " << postings.size() << " words" << endl;
    cout << "Total documents: " << totalDocuments << ", Avg doc length: " << avgDocLength << endl;
}

void loadDocuments(const string &path)
{
    ifstream file(path);
    if (!file.is_open())
    {
        cerr << "Warning: Could not open documents at " << path << endl;
        return;
    }

    string line;
    getline(file, line); // Skip header

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        // Simple CSV parsing (handles basic cases)
        vector<string> cols;
        stringstream ss(line);
        string col;

        while (getline(ss, col, ','))
        {
            cols.push_back(col);
        }

        if (cols.size() >= 5)
        {
            Document doc;
            doc.docId = cols[0];
            doc.authors = cols[2];
            doc.title = cols[3];
            doc.abstract = cols[4];
            documents[doc.docId] = doc;
        }
    }

    cout << "Loaded " << documents.size() << " documents" << endl;
}

void loadDocUrls(const string &path)
{
    ifstream file(path);
    if (!file.is_open())
    {
        cerr << "Warning: Could not open doc_urls at " << path << endl;
        return;
    }

    string line;
    getline(file, line); // Skip header

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        size_t commaPos = line.find(',');
        if (commaPos != string::npos)
        {
            string docId = line.substr(0, commaPos);
            string url = line.substr(commaPos + 1);
            docUrls[docId] = url;
        }
    }

    cout << "Loaded " << docUrls.size() << " document URLs" << endl;
}

// ============================================
// BM25 SEARCH FUNCTION
// Industry-standard semantic ranking algorithm
// Considers term frequency, document frequency,
// and document length for relevance scoring
// ============================================

// Calculate IDF (Inverse Document Frequency) for a term
double calculateIDF(int docFreq)
{
    if (docFreq <= 0 || totalDocuments <= 0)
        return 0.0;
    // Standard IDF formula with smoothing
    return log((totalDocuments - docFreq + 0.5) / (docFreq + 0.5) + 1.0);
}

// Calculate BM25 score for a term in a document
double calculateBM25Score(int termFreq, int docLength, double idf)
{
    // BM25 formula
    double tf = (double)termFreq;
    double docLenNorm = (double)docLength / avgDocLength;
    double numerator = tf * (k1 + 1);
    double denominator = tf + k1 * (1 - b + b * docLenNorm);
    return idf * (numerator / denominator);
}

vector<SearchResult> search(const string &query)
{
    vector<string> terms = tokenize(query);
    unordered_map<string, double> scores;   // docId -> BM25 score
    unordered_map<string, int> termMatches; // docId -> number of query terms matched

    // Remove duplicate terms from query
    unordered_set<string> uniqueTerms(terms.begin(), terms.end());

    // For each unique search term
    for (const string &term : uniqueTerms)
    {
        auto lexIt = lexicon.find(term);
        if (lexIt == lexicon.end())
            continue; // Word not in lexicon

        int wordId = lexIt->second;
        auto postIt = postings.find(wordId);
        if (postIt == postings.end())
            continue; // No postings

        // Get IDF for this term
        double idf = calculateIDF(docFrequency[wordId]);

        // Calculate BM25 score for each document containing this term
        for (const auto &posting : postIt->second)
        {
            const string &docId = posting.first;
            int termFreq = posting.second;
            int docLength = docLengths.count(docId) ? docLengths[docId] : (int)avgDocLength;

            double bm25Score = calculateBM25Score(termFreq, docLength, idf);
            scores[docId] += bm25Score;
            termMatches[docId]++;
        }
    }

    // Convert to vector and apply coordination factor boost
    vector<SearchResult> results;
    int queryTermCount = uniqueTerms.size();

    for (const auto &p : scores)
    {
        SearchResult result;
        result.docId = p.first;

        // Apply coordination factor: documents matching more query terms get boosted
        double coordFactor = queryTermCount > 0 ? (double)termMatches[p.first] / queryTermCount : 1.0;
        result.score = p.second * (0.5 + 0.5 * coordFactor);

        // Get document metadata if available
        auto docIt = documents.find(p.first);
        if (docIt != documents.end())
        {
            result.title = docIt->second.title;
            result.authors = docIt->second.authors;
            result.abstract = docIt->second.abstract;
        }
        else
        {
            result.title = "Document " + p.first;
            result.authors = "";
            result.abstract = "";
        }

        // Get URL if available
        auto urlIt = docUrls.find(p.first);
        if (urlIt != docUrls.end())
        {
            result.url = urlIt->second;
        }

        results.push_back(result);
    }

    // Sort by score (highest first)
    sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b)
         { return a.score > b.score; });

    // Return top 20 results
    if (results.size() > 20)
    {
        results.resize(20);
    }

    return results;
}

// ============================================
// JSON HELPERS
// ============================================

string escapeJson(const string &s)
{
    string result;
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += c;
        }
    }
    return result;
}

string resultsToJson(const vector<SearchResult> &results)
{
    stringstream json;
    json << "{\"results\":[";

    for (size_t i = 0; i < results.size(); i++)
    {
        if (i > 0)
            json << ",";
        json << "{";
        json << "\"docId\":\"" << escapeJson(results[i].docId) << "\",";
        json << "\"title\":\"" << escapeJson(results[i].title) << "\",";
        json << "\"authors\":\"" << escapeJson(results[i].authors) << "\",";
        json << "\"abstract\":\"" << escapeJson(results[i].abstract) << "\",";
        json << "\"url\":\"" << escapeJson(results[i].url) << "\",";
        json << "\"score\":" << results[i].score;
        json << "}";
    }

    json << "]}";
    return json.str();
}

string suggestionsToJson(const vector<string> &suggestions)
{
    stringstream json;
    json << "{\"suggestions\":[";
    for (size_t i = 0; i < suggestions.size(); i++)
    {
        if (i > 0)
            json << ",";
        json << "\"" << escapeJson(suggestions[i]) << "\"";
    }
    json << "]}";
    return json.str();
}

// ============================================
// HTTP SERVER
// ============================================

void handleClient(SOCKET clientSocket)
{
    char buffer[4096] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);

    string request(buffer);
    string response;
    string body;

    // CORS headers for React frontend
    string corsHeaders =
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, ngrok-skip-browser-warning\r\n";

    // Handle OPTIONS preflight
    if (request.find("OPTIONS") == 0)
    {
        response = "HTTP/1.1 204 No Content\r\n" + corsHeaders + "\r\n";
    }
    // Handle autocomplete request
    else if (request.find("GET /autocomplete") != string::npos)
    {
        string prefix = getQueryParam(request);
        auto suggestions = autocompleteTrie.autocomplete(prefix, 8);
        body = suggestionsToJson(suggestions);

        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n" +
                   corsHeaders +
                   "Content-Length: " + to_string(body.length()) + "\r\n"
                                                                   "\r\n" +
                   body;
    }
    // Handle search request
    else if (request.find("GET /search") != string::npos)
    {
        string query = getQueryParam(request);

        // Measure search time
        auto startTime = chrono::high_resolution_clock::now();

        auto results = search(query);

        auto searchEnd = chrono::high_resolution_clock::now();
        auto searchMs = chrono::duration_cast<chrono::microseconds>(searchEnd - startTime).count() / 1000.0;

        body = resultsToJson(results);

        auto jsonEnd = chrono::high_resolution_clock::now();
        auto jsonMs = chrono::duration_cast<chrono::microseconds>(jsonEnd - searchEnd).count() / 1000.0;

        cout << "Query: \"" << query << "\" | Search: " << searchMs << "ms | JSON: " << jsonMs << "ms | Results: " << results.size() << endl;

        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n" +
                   corsHeaders +
                   "Content-Length: " + to_string(body.length()) + "\r\n"
                                                                   "\r\n" +
                   body;
    }
    // 404 for other requests
    else
    {
        body = "{\"error\":\"Not Found\"}";
        response = "HTTP/1.1 404 Not Found\r\n"
                   "Content-Type: application/json\r\n" +
                   corsHeaders +
                   "Content-Length: " + to_string(body.length()) + "\r\n"
                                                                   "\r\n" +
                   body;
    }

    send(clientSocket, response.c_str(), response.length(), 0);
    closesocket(clientSocket);
}

int main()
{
    cout << "========================================" << endl;
    cout << "   CORD-19 Search Engine API Server    " << endl;
    cout << "========================================" << endl;

    // Load data
    cout << "\nLoading data..." << endl;
    loadLexicon("data/lexicon.csv");
    loadPostings("data/postings.csv");
    loadDocuments("Code Produced Data/cord_processed.csv");
    loadDocUrls("data/doc_urls.csv");

// Initialize Winsock (Windows only)
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }
#endif

    // Create socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    // Allow port reuse
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    // Bind to port 5000
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(5000);

    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "Bind failed" << endl;
        closesocket(serverSocket);
        return 1;
    }

    // Listen for connections
    if (listen(serverSocket, 10) == SOCKET_ERROR)
    {
        cerr << "Listen failed" << endl;
        closesocket(serverSocket);
        return 1;
    }

    cout << "\nServer running on http://localhost:5000" << endl;
    cout << "Press Ctrl+C to stop\n"
         << endl;

    // Accept and handle connections
    while (true)
    {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket != INVALID_SOCKET)
        {
            handleClient(clientSocket);
        }
    }

    closesocket(serverSocket);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
