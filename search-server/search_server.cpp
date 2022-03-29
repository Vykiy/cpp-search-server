#include "search_server.h"
#include "string_processing.h"

#include <cmath>
#include <numeric>
#include <stdexcept>

using namespace std::string_literals;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id,
                               const std::string& document,
                               DocumentStatus status,
                               const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("ID документа отрицательный"s);
    }

    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("ID документа совпадает с уже имеющимся"s);
    }

    if (IsValidText(document)) {
        const std::vector<std::string> words = SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size();
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
        ids_documents_.push_back(document_id);
    }
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query,
                                                     DocumentStatus document_status) const {
    return FindTopDocuments(raw_query,
                            [document_status](int document_id, DocumentStatus status, int rating) {
                                return status == document_status;
    });
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                                                 int document_id) const {
    std::tuple<std::vector<std::string>, DocumentStatus> result;

    if (IsValidText(raw_query)) {
        Query query;

        if (ParseQuery(raw_query, query)) {
            std::vector<std::string> matched_words;
            for (const std::string& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }

            for (const std::string& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.clear();
                    break;
                }
            }

            result = {matched_words, documents_.at(document_id).status};
        }
    }
    return result;
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    if (index < 0 || index > GetDocumentCount() - 1)
    {
       throw std::out_of_range("Индекс документа вне диапазона"s);
    }
    return ids_documents_[index];
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

bool SearchServer::ParseQueryWord(std::string text, QueryWord& query_word) const {
    bool is_minus = false;

    if (text[0] == '-' && text.size() > 1) {
        is_minus = true;
        text = text.substr(1);
    }

    if (text[0] == '-' || text[text.size() - 1] == '-')
    {
        throw std::invalid_argument("Поисковый запрос некорректен"s);
    }

    query_word = {text, is_minus, IsStopWord(text)};
    return true;
}

bool SearchServer::ParseQuery(const std::string& text, Query& query) const {
    for (const std::string& word : SplitIntoWords(text)) {
        QueryWord query_word;
        if (ParseQueryWord(word, query_word)){
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
    }
    return true;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
