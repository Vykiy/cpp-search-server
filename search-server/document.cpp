#include "document.h"

using namespace std::string_literals;

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
   : id(id)
   , relevance(relevance)
   , rating(rating) {
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
              << "document_id = "s << document.id << ", "s
              << "relevance = "s << document.relevance << ", "s
              << "rating = "s << document.rating << " }"s << std::endl;
}

std::ostream& operator<<(std::ostream& output, Document document) {
    output << "{ document_id = "s << document.id
           << ", relevance = "s << document.relevance
           << ", rating = "s << document.rating << " }"s;
    return output;
}
