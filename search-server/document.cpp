#include "document.h"

using std::ostream;
using namespace std::string_literals;

Document::Document (int id, double relevance, int rating)
        : id(id), relevance(relevance), rating(rating) {
}

ostream &operator<< (ostream &out, const Document &document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

void PrintDocument (const Document &document) {
    std::cout << "{ "s
              << "document_id = "s << document.id << ", "s
              << "relevance = "s << document.relevance << ", "s
              << "rating = "s << document.rating << " }"s << '\n';
}

void PrintMatchDocumentResult (int document_id, const std::vector<std::string> &words, DocumentStatus status) {
    std::cout << "{ "s
              << "document_id = "s << document_id << ", "s
              << "status = "s << static_cast<int>(status) << ", "s
              << "words ="s;
    for (const std::string &word: words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << '\n';
}