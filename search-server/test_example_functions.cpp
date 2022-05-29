#include "test_example_functions.h"

using namespace std;

void AddDocument(SearchServer& search_server, int document_id, string_view document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const invalid_argument& e) {
        cout << "Error when adding document "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, string_view raw_query) {
    cout << "Search result for the query: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const invalid_argument& e) {
        cout << "Search error: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, string_view query) {
    try {
        cout << "Matching documents on query: "s << query << endl;
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }

    } catch (const invalid_argument& e) {
        cout << "Error matching documents on query "s << query << ": "s << e.what() << endl;
    }
}

