#include "search_server.h"

using namespace std;


SearchServer::SearchServer(string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(const string& stop_words_text)
: SearchServer(SplitIntoWords(stop_words_text)) {
}



void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string_view word : words) {
        word_to_document_freqs_[string(word)][document_id] += inv_word_count;
        id_word_frequencies_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}


vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const { //***
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const { //***
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

const set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string_view, double> get_map;
    if (id_word_frequencies_.count(document_id)) {
        return id_word_frequencies_.at(document_id);
    }
    return get_map;
}

void SearchServer::RemoveDocument(int document_id){
    const auto doc_found_it = find(document_ids_.begin(), document_ids_.end(), document_id);
    //const auto doc_found_it = document_ids_.find(document_id);

    if (doc_found_it == document_ids_.end()) {
        return;
    }

    document_ids_.erase(doc_found_it);
    documents_.erase(document_id);

    for (auto& [word, d] : id_word_frequencies_.at(document_id)) {
        word_to_document_freqs_.at(string(word)).erase(document_id);
    }

    id_word_frequencies_.erase(document_id);
}


void SearchServer::RemoveDocument(const execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const execution::parallel_policy&, int document_id) {
        if (documents_.count(document_id) == 0) {
            return;
        }

        documents_.erase(document_id);
        auto it = std::find(execution::par, document_ids_.begin(), document_ids_.end(), document_id);
        if (it != document_ids_.end()) {
            document_ids_.erase(it);
        }
    }


tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
    return MatchDocument(execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, string_view raw_query, int document_id) const {
    std::vector<std::string_view> matched_words;
    if ((document_id < 0) || (documents_.count(document_id)==0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    if (!IsValidWord(raw_query)) {
        throw invalid_argument("Some of query words are invalid"s);
    }
    const auto query = ParseQuery(raw_query);
    for (const string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
    }

    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, true);
    vector<string_view> matched_words;
    const auto status = documents_.at(document_id).status;
    const auto word_checker   =
        [this, document_id](const string_view word) {
        const auto it = word_to_document_freqs_.find(string(word));
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };
    if (any_of(execution::par, query.minus_words.begin(), query.minus_words.end(), word_checker )) {
        return { matched_words, status };
    }

    matched_words.reserve(query.plus_words.size());
    auto words_end = copy_if(execution::par, query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(), word_checker
    );
    sort(matched_words.begin(), words_end);
    words_end = unique(matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return { matched_words, status };
}


bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}


int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid"s);
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(string_view text, bool sort) const {
    Query result;
    for (const string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!sort) {
        for (auto* words : { &result.plus_words, &result.minus_words }) {
            std::sort(words->begin(), words->end());
            words->erase(unique(words->begin(), words->end()), words->end());
        }
    }
    return result;
}


double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(string(word)).size());
}
