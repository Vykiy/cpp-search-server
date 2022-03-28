#pragma once
#include "search_server.h"

#include <deque>
#include <string>
#include <vector>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query,
                                         DocumentPredicate document_predicate) {
        auto matched_documents = search_server_.FindTopDocuments(raw_query, document_predicate);

        AddRequest(matched_documents.size());

        return matched_documents;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query,
                                         DocumentStatus status = DocumentStatus::ACTUAL);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        uint64_t timestamp;
        int results;
    };

    std::deque<QueryResult> requests_;
    const SearchServer& search_server_;
    const static int min_in_day_ = 1440;
    int no_result_request_count_;
    uint64_t current_time_;

    void AddRequest(int results_num);
};
