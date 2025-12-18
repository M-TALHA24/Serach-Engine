// ============================================
// SEARCH API SERVER
// A simple HTTP server that handles search requests
// Returns JSON results to the React frontend
// ============================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>

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
};

// Structure for search results with ranking score
struct SearchResult
{
    string docId;
    string title;
    string authors;
    string abstract;
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
Trie autocompleteTrie;                                  // Trie for word suggestions

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

        for (size_t i = 0; i < docs.size() && i < frequencies.size(); i++)
        {
            postings[wordId].push_back({docs[i], frequencies[i]});
        }
    }

    cout << "Loaded postings for " << postings.size() << " words" << endl;
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

// ============================================
// SEARCH FUNCTION
// Simple TF-based ranking
// ============================================

vector<SearchResult> search(const string &query)
{
    vector<string> terms = tokenize(query);
    unordered_map<string, double> scores; // docId -> score

    // For each search term
    for (const string &term : terms)
    {
        auto lexIt = lexicon.find(term);
        if (lexIt == lexicon.end())
            continue; // Word not in lexicon

        int wordId = lexIt->second;
        auto postIt = postings.find(wordId);
        if (postIt == postings.end())
            continue; // No postings

        // Add frequency to document score
        for (const auto &posting : postIt->second)
        {
            scores[posting.first] += posting.second;
        }
    }

    // Convert to vector and sort by score
    vector<SearchResult> results;
    for (const auto &p : scores)
    {
        SearchResult result;
        result.docId = p.first;
        result.score = p.second;

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
        "Access-Control-Allow-Headers: Content-Type\r\n";

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
        cout << "Search query: " << query << endl;

        auto results = search(query);
        body = resultsToJson(results);

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
