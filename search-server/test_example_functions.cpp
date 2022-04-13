#include "test_example_functions.h"

void AddDocument (SearchServer &search_server, const int &id, const string &document,
                  DocumentStatus status, const vector<int> &rating) {
    try {
        search_server.AddDocument(id, document, status, rating);
    } catch (const std::exception &e) {
        std::cerr << "Ошибка при добавлении нового документа: " << id << ": "s << e.what() << '\n';
    }
}

void FindTopDocuments (const SearchServer &search_server, const string &raw_query) {
    std::cout << "Результаты поиска по запросу: "s << raw_query << '\n';
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка при поиске: "s << e.what() << '\n';
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        std::cout << "Матчинг документов по запросу: "s << query << '\n';
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка при матчинге документов по запросу "s << query << ": "s << e.what() << '\n';
    }
}
