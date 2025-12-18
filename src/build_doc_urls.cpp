#include <fstream>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

// Proper CSV parser (handles quotes)
vector<string> parseCSV(const string &line)
{
    vector<string> cols;
    string cur;
    bool inQuotes = false;

    for (char c : line)
    {
        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == ',' && !inQuotes)
        {
            cols.push_back(cur);
            cur.clear();
        }
        else
        {
            cur += c;
        }
    }
    cols.push_back(cur);
    return cols;
}

int main()
{
    ifstream meta(
        "cord-19_2020-05-26/2020-05-26/metadata.csv");
    ofstream out("data/doc_urls.csv");

    if (!meta.is_open())
    {
        cerr << "❌ metadata.csv not found\n";
        return 1;
    }

    out << "docID,url\n";

    string header;
    getline(meta, header);
    auto headers = parseCSV(header);

    int docID_col = -1;
    int url_col = -1;

    for (int i = 0; i < headers.size(); i++)
    {
        if (headers[i] == "cord_uid")
            docID_col = i;
        if (headers[i] == "url")
            url_col = i;
    }

    if (docID_col == -1 || url_col == -1)
    {
        cerr << "❌ cord_uid or url column not found\n";
        return 1;
    }

    string line;
    int count = 0;

    while (getline(meta, line))
    {
        auto cols = parseCSV(line);
        if (cols.size() <= max(docID_col, url_col))
            continue;

        string docID = cols[docID_col];
        string url = cols[url_col];

        if (!docID.empty() && !url.empty())
        {
            out << docID << "," << url << "\n";
            count++;
        }
    }

    cout << "✅ doc_urls.csv built correctly (" << count << " URLs)\n";
    return 0;
}
