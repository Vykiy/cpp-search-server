#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <iomanip>


using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ETALON_RELEVANCE = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}



int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};



enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < ETALON_RELEVANCE) {
                     return lhs.rating > rhs.rating;
                 }
                 else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;

    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query,
                                [status](int document_id, DocumentStatus status_pr, int rating) { return status_pr == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    /*int rating;
    DocumentStatus status;*/
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_; //хранит id документа, рейтинг и статус

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        // int tt= rating_sum / static_cast<int>(ratings.size());
        return rating_sum / static_cast<int>(ratings.size());
        //return tt;
    }

    //string data;
    //bool is_minus;
    //bool is_stop;
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1); //substr(1) - обрезает первый символ.
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    /*set<string> plus_words;
    set<string> minus_words;*/
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Predikat>
    vector<Document> FindAllDocuments(const Query& query, Predikat filter) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                auto doc_filter = documents_.at(document_id);
                if (filter(document_id, doc_filter.status, doc_filter.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating
                                        });
        }
        return matched_documents;
    }
};

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/


template<typename Element>
ostream& operator<<(ostream& out, const vector<Element>& v) {
    for (const auto& element : v) {
        out << element;
    }
    return out;
}

template<typename Element1>
ostream& operator<<(ostream& out, const Document& s) {
    out << s.id << s.relevance << s.rating;
    return out;
}


void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}
//#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(a) a()

// -------- Начало модульных тестов поисковой системы ----------
//void PrintDocument(const Document& document) {
//    cout << "{ "s
//        << "document_id = "s << document.id << ", "s
//        << "relevance = "s << document.relevance << ", "s
//        << "rating = "s << document.rating
//        << " }"s << endl;
//}


void TestExcludeStopWordsFromAddedDocumentContent () {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        assert(found_docs.size() == 1);
        const Document &doc0 = found_docs[0];
        assert(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        assert(server.FindTopDocuments("in"s).empty());
    }
}

// 1.проверят, что документ должен находиться по поисковому запросу, который содержит слова из документа
void TestAfterCheckAddDocumentByDocumentSafeAndReturnRightDocument(){
    const int id_first = 1, id_second = 2, id_third = 20;
    const string content_first = "Masha went to the forest and met many midi animals"s;
    const string content_second = "My cat are very active at night and during the day he likes to sleep"s;
    const string content_third = "Programmers are like cats"s;
    const vector<int> ratings_first = {1, 2, 3}, ratings_second = {10, -2, 300}, ratings_third = {1000, 200, 300};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);

    assert(server.GetDocumentCount() == 3);
    server.SetStopWords("are at the to"s);

    const string query_first = "Hi guys"s;
    const string query_second = "Hi cats"s;
    {
        assert(server.FindTopDocuments(query_first).empty());
        assert(server.FindTopDocuments(query_second).size() == 1);
        const auto find_docs = server.FindTopDocuments(query_second);
        assert(find_docs[0].id == 20);
    }
    {
        server.AddDocument(1000, "spessial information"s, DocumentStatus::ACTUAL, ratings_first);
        const auto find_docs = server.FindTopDocuments("spessial information"s);
        assert(!server.FindTopDocuments(query_second).empty());
        assert(find_docs[0].id == 1000);
    }

}

// 3.Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestDropMinusWordsFromDocuments(){
    const int id_first = 1, id_second = 2, id_third = 20;
    const string content_first = "Masha went to the forest and met many midi animals"s;
    const string content_second = "My cat are very active at night and during the day he likes to sleep"s;
    const string content_third = "Programmers are like cats"s;
    const vector<int> ratings_first = {1, 2, 3}, ratings_second = {10, -2, 300}, ratings_third = {1000, 200, 300};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);

    assert(server.GetDocumentCount() == 3);
    server.SetStopWords("are at the to"s);

    assert(server.FindTopDocuments("Programmers -cats"s).empty());

}

// 4.При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе.
// Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchingQueryWordsFromDocument(){
    const int id_first = 1, id_second = 2, id_third = 20;
    const string content_first = "Masha went to the forest and met many midi animals"s;
    const string content_second = "My cat are very active at night and during the day he likes to sleep"s;
    const string content_third = "Programmers are like cats"s;
    const vector<int> ratings_first = {1, 2, 3}, ratings_second = {10, -2, 300}, ratings_third = {1000, 200, 300};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);

    //tuple<vector<string>, DocumentStatus> MatchDocument (const string &raw_query, int document_id) const
    const auto test_first = server.MatchDocument("Masha went to the forest and met many animals -midi animals", 1);
    assert(get<0>(test_first).empty());

    const auto test_second = server.MatchDocument("Masha"s, 1);
    assert(get<0>(test_second).size() == 1);
}

// 5.Сортировка найденных документов по релевантности.
// Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortByRelevance(){
    const int id_first = 0, id_second = 1, id_third = 2;
    const string content_first = "pasha masha yandex"s;
    const string content_second = "pasha masha cats "s;
    const string content_third = "only masha and cat"s;
    const vector<int> ratings_first = {8, -3}, ratings_second = {7, 2, 0}, ratings_third = {-1, -2};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);

    const auto result_documents = server.FindTopDocuments("pasha masha yandex"s);
    double begin = 0, end = 0;
    for (const auto& document : result_documents) {
        if(begin == 0 && end == 0)
        {
            begin = end = document.relevance;
        }
        else
        {
            end = document.relevance;
        }
        ASSERT((begin >=  end));
        begin = end;
    }
}

// 6.Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestCheckCalculateRating(){
    const int id_first = 1, id_second = 2, id_third = 3;
    const string content_first = "pasha masha yandex"s;
    const string content_second = "pasha masha cats "s;
    const string content_third = "only masha and cat"s;
    const vector<int> ratings_first = {}, ratings_second = {7, 2, 0}, ratings_third = {-1, -2};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);

    const auto result_documents = server.FindTopDocuments("pasha masha yandex"s);
    assert(result_documents.size() == 3);
    assert(result_documents[0].rating == 0);
    assert(result_documents[1].rating == 3);
    assert(result_documents[2].rating == -1);
}

// 7.Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestSearchWithPredicate(){
    const int id_first = 1, id_second = 2, id_third = 3;
    const string content_first = "pasha masha yandex"s;
    const string content_second = "pasha masha cats "s;
    const string content_third = "only masha and cat"s;
    const vector<int> ratings_first = {0, 2, 7}, ratings_second = {0}, ratings_third = {-1, -1, -1};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);
    {
        const auto first_result = server.FindTopDocuments("only masha and cat",
                                                          [](int document_id, DocumentStatus status, int rating){return document_id % 2 == 0;});
        assert(first_result.size() == 1);
    }

    {
        const auto first_result = server.FindTopDocuments("only masha and cat",
                                                          [](int document_id, DocumentStatus status, int rating){return document_id  == 1;});
        assert(first_result.size() == 1);
    }

    {
        const auto first_result = server.FindTopDocuments("only masha and cat",
                                                          [](int document_id, DocumentStatus status, int rating){return rating > 1000000;});
        assert(first_result.empty());
    }

}

// 8.Поиск документов, имеющих заданный статус
void TestFindDocsWithStatus(){
    const int id_first = 1, id_second = 2, id_third = 3;
    const string content_first = "pasha masha yandex"s;
    const string content_second = "pasha masha cats "s;
    const string content_third = "only masha and cat"s;
    const string content_ble = "very interesting information"s;
    const vector<int> ratings_first = {0, 2, 7}, ratings_second = {0}, ratings_third = {-1, -1, -1};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::IRRELEVANT, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::BANNED, ratings_third);
    server.AddDocument(5, content_ble, DocumentStatus::REMOVED, {1000, 10000, 100000});
    {
        const auto first_result = server.FindTopDocuments("only masha and cat",DocumentStatus::REMOVED);
        assert(first_result.empty());
    }
    {
        const auto first_result = server.FindTopDocuments("very interesting information",DocumentStatus::REMOVED);
        assert(first_result.size() == 1);
    }
    {
        const auto first_result = server.FindTopDocuments("pasha masha",DocumentStatus::IRRELEVANT);
        assert(first_result.size() == 1);
    }
    {
        const auto first_result = server.FindTopDocuments("yandex",DocumentStatus::ACTUAL);
        assert(first_result.size() == 1);
    }

}

// 9.Корректное вычисление релевантности найденных документов.
void TestCountRelevance(){
    const int id_first = 0, id_second = 1, id_third = 2;
    const string content_first = "pasha pasha pasha masha yandex cat"s;
    const string content_second = "pasha pasha masha yandex cat"s;
    const string content_third = "pasha masha and cat"s;
    const vector<int> ratings = { 1, 2, 3};
    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings);
    const auto doc = server.FindTopDocuments("pasha"s);
    double idf = log(static_cast<double>(3) / 2);
    //Ошибка. TestCountRelevance: ASSERT((abs(doc[0].relevance - idf) < ETALON_R
    //ELEVANCE)) failed.
    ASSERT(((doc[0].relevance - idf) < ETALON_RELEVANCE));
    ASSERT(((doc[1].relevance - 0.5 * idf) < ETALON_RELEVANCE));
    ASSERT(((doc[2].relevance - 0.25 * idf) < ETALON_RELEVANCE));
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer () {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestAfterCheckAddDocumentByDocumentSafeAndReturnRightDocument();
    TestDropMinusWordsFromDocuments();
    TestMatchingQueryWordsFromDocument();
    TestSortByRelevance();
    TestCheckCalculateRating();
    TestSearchWithPredicate();
    TestFindDocsWithStatus();
    TestCountRelevance();
}

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}