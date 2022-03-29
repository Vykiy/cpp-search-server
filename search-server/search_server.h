#pragma once
#include "document.h"
#include "string_processing.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id,
                     const std::string& document,
                     DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query,
                                           DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query,
                                           DocumentStatus document_status = DocumentStatus::ACTUAL) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
                                                                       int document_id) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> ids_documents_;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    bool ParseQueryWord(std::string text, QueryWord& query_word) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    bool ParseQuery(const std::string& text, Query& query) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
                                           DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query,
                                                     DocumentPredicate document_predicate) const {
    std::vector<Document> result;

    if (IsValidText(raw_query)) {
        Query query;
        if (ParseQuery(raw_query, query)) {
            result = FindAllDocuments(query, document_predicate);

            sort(result.begin(), result.end(),
                 [](const Document& lhs, const Document& rhs) {
                    if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                        return lhs.rating > rhs.rating;
                    }
                    return lhs.relevance > rhs.relevance;
            });

            if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
                result.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
        }
    }
    return result;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                                     DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;

    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& documents : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(documents.first);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }

    return matched_documents;
}
