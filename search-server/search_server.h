#pragma once

#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <set>
#include <map>
#include <stdexcept>
#include <execution>
#include <thread>
#include <iterator>

#include "string_processing.h"
#include "document.h"
#include "paginator.h"
#include "concurrent_map.h"



using namespace std::string_literals;


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double relevance_deviation = 1e-6;


class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);

    const std::set<int>::const_iterator begin() const;
    const std::set<int>::const_iterator end() const;


    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);


    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;


    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const;


    int GetDocumentCount() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);


    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&,
                                                                            std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&,
                                                                            std::string_view raw_query, int document_id) const;


private:
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> id_word_frequencies_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text, bool sort = false) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const;


    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

//-----------------------------------------FindTopDocuments--------------------------------------------//
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
              [](const Document& lhs, const Document& rhs) {

        if (std::abs(lhs.relevance - rhs.relevance) < relevance_deviation) {
            return lhs.rating > rhs.rating;

        } else {
            return lhs.relevance > rhs.relevance;
        }
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}


template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                                     std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(std::execution::par,
              matched_documents.begin(), matched_documents.end(),
              [](const Document& lhs, const Document& rhs) {

        if (std::abs(lhs.relevance - rhs.relevance) < relevance_deviation) {
            return lhs.rating > rhs.rating;

        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}


template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                                     std::string_view raw_query,
                                                     DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


//-----------------------------------------FindAllDocuments--------------------------------------------//

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                                     DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        
        for (const auto [id, freq] : word_to_document_freqs_.at(std::string(word))) {
            const auto& document_data = documents_.at(id);
            
            if (document_predicate(id, document_data.status, document_data.rating)) {
                document_to_relevance[id] += freq * ComputeWordInverseDocumentFreq(std::string(word));
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto [id, _] : word_to_document_freqs_.at(std::string(word))) {
            document_to_relevance.erase(id);
        }
    }


    std::vector<Document> matched_documents;
    for (const auto [id, relevance] : document_to_relevance) {
        matched_documents.push_back({id, relevance, documents_.at(id).rating});
    }

    return matched_documents;
}


template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy,
                                                     const Query& query,
                                                     DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(query.plus_words.size());

    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](std::string_view word) {
        if (word_to_document_freqs_.count(std::string(word)) > 0) {

            for (const auto [id, freq] : word_to_document_freqs_.at(std::string(word))) {
                const auto& document_data = documents_.at(id);

                if (document_predicate(id, document_data.status, document_data.rating)) {
                    document_to_relevance[id].ref_to_value += freq * ComputeWordInverseDocumentFreq(std::string(word));
                }
            }
        }
    });

    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](std::string_view word) {
        if (word_to_document_freqs_.count(std::string(word)) > 0) {
            for (const auto [id, _] : word_to_document_freqs_.at(std::string(word))) {
                document_to_relevance.BuildOrdinaryMap().erase(id);
            }
        }
    });

    std::vector<Document> matched_documents;
    for (const auto& [id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({ id, relevance, documents_.at(id).rating });
    }

    return matched_documents;
}
