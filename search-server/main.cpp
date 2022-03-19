#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

/* 
   Подставьте сюда вашу реализацию макросов 
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

// -------- Начало модульных тестов поисковой системы ----------


void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
//Разместите код остальных тестов здесь
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

    //assert(server.GetDocumentCount() == 3);
    ASSERT_EQUAL(server.GetDocumentCount(), 3);
    server.SetStopWords("are at the to"s);

    const string query_first = "Hi guys"s;
    const string query_second = "Hi cats"s;
    {
        //assert(server.FindTopDocuments(query_first).empty());
        ASSERT_HINT(server.FindTopDocuments(query_first).empty(),"dont do that"s);
        //assert(server.FindTopDocuments(query_second).size() == 1);
        ASSERT_EQUAL(server.FindTopDocuments(query_second).size(), 1);
        const auto find_docs = server.FindTopDocuments(query_second);
        //assert(find_docs[0].id == 20);
        ASSERT_EQUAL(find_docs[0].id,20);
    }
    {
        server.AddDocument(1000, "spessial information"s, DocumentStatus::ACTUAL, ratings_first);
        const auto find_docs = server.FindTopDocuments("spessial information"s);
        //assert(!server.FindTopDocuments(query_second).empty());
        ASSERT_HINT(!server.FindTopDocuments(query_second).empty(),"dont do that"s);
        //assert(find_docs[0].id == 1000);
        ASSERT_EQUAL(find_docs[0].id,1000);
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

    //assert(server.GetDocumentCount() == 3);
    ASSERT_EQUAL(server.GetDocumentCount(), 3);
    server.SetStopWords("are at the to"s);
    //assert(server.FindTopDocuments("Programmers -cats"s).empty());
    ASSERT_HINT(server.FindTopDocuments("Programmers -cats"s).empty(),"dont do that");

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
    //assert(get<0>(test_first).empty());
    ASSERT_HINT(get<0>(test_first).empty(), "dont do that"s);

    const auto test_second = server.MatchDocument("Masha"s, 1);
    //assert(get<0>(test_second).size() == 1);
    ASSERT_EQUAL(get<0>(test_second).size(),1);
}

// 5.Сортировка найденных документов по релевантности.
// Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortByRelevance(){
    const int id_first = 1, id_second = 2, id_third = 3;
    const string content_first = "pasha masha yandex"s;
    const string content_second = "pasha masha cats "s;
    const string content_third = "only masha and cat"s;
    const vector<int> ratings_first = {0, 2, 7}, ratings_second = {0}, ratings_third = {-1, -1, -1};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);

    const auto result_documents = server.FindTopDocuments("pasha masha yandex"s);
    //assert(result_documents.size() == 3);
    //assert(result_documents[0].id == 1);
    //assert(result_documents[1].id == 2);
    //assert(result_documents[2].id == 3);
    ASSERT_EQUAL(result_documents.size(), 3);
    ASSERT_EQUAL(result_documents[0].id, 1);
    ASSERT_EQUAL(result_documents[1].id, 2);
    ASSERT_EQUAL(result_documents[2].id, 3);
}

// 6.Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestCheckCalculateRaiting(){
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
    //assert(result_documents.size() == 3);
    //assert(result_documents[0].rating == 0);
    //assert(result_documents[1].rating == 3);
    //assert(result_documents[2].rating == -1);
    ASSERT_EQUAL(result_documents.size(),3);
    ASSERT_EQUAL(result_documents[0].rating,0);
    ASSERT_EQUAL(result_documents[1].rating,3);
    ASSERT_EQUAL(result_documents[2].rating,-1);
}

// 7.Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestCherchWithPredicate(){
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
                                                          [](int document_id, [[maybe_unused]]DocumentStatus status, [[maybe_unused]]int rating){return document_id % 2 == 0;});
        //assert(first_result.size() == 1);
        ASSERT_EQUAL(first_result.size(), 1);
    }

    {
        const auto first_result = server.FindTopDocuments("only masha and cat",
                                                          [](int document_id, [[maybe_unused]]DocumentStatus status, [[maybe_unused]]int rating){return document_id  == 1;});
        //assert(first_result.size() == 1);
        ASSERT_EQUAL(first_result.size(), 1);
    }

    {
        const auto first_result = server.FindTopDocuments("only masha and cat",
                                                          []([[maybe_unused]]int document_id, [[maybe_unused]]DocumentStatus status, [[maybe_unused]]int rating){return rating > 1000000;});
        //assert(first_result.empty());
        ASSERT_HINT(first_result.empty(),"dont do that"s);
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
        //assert(first_result.empty());
        ASSERT_HINT(first_result.empty(),"dont do that"s);
    }
    {
        const auto first_result = server.FindTopDocuments("very interesting information",DocumentStatus::REMOVED);
        //assert(first_result.size() == 1);
        ASSERT_EQUAL(first_result.size(),1);
    }
    {
        const auto first_result = server.FindTopDocuments("pasha masha",DocumentStatus::IRRELEVANT);
        //assert(first_result.size() == 1);
        ASSERT_EQUAL(first_result.size(),1);
    }
    {
        const auto first_result = server.FindTopDocuments("yandex",DocumentStatus::ACTUAL);
        //assert(first_result.size() == 1);
        ASSERT_EQUAL(first_result.size(),1);
    }
}

// 9.Корректное вычисление релевантности найденных документов.
void TestCountRelevanse(){
    const int id_first = 1, id_second = 2, id_third = 3;
    const string content_first = "pasha masha yandex"s;
    const string content_second = "pasha masha cats "s;
    const string content_third = "only masha and cat"s;
    const string content_ble = "very interesting information"s;
    const vector<int> ratings_first = {0, 2, 7}, ratings_second = {0}, ratings_third = {-1, -1, -1};

    SearchServer server;
    server.AddDocument(id_first, content_first, DocumentStatus::ACTUAL, ratings_first);
    server.AddDocument(id_second, content_second, DocumentStatus::ACTUAL, ratings_second);
    server.AddDocument(id_third, content_third, DocumentStatus::ACTUAL, ratings_third);
    server.AddDocument(5, content_ble, DocumentStatus::ACTUAL, {1000, 10000, 100000});
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestAfterCheckAddDocumentByDocumentSafeAndReturnRightDocument);
    RUN_TEST(TestDropMinusWordsFromDocuments);
    RUN_TEST(TestMatchingQueryWordsFromDocument);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestCheckCalculateRaiting);
    RUN_TEST(TestCherchWithPredicate);
    RUN_TEST(TestFindDocsWithStatus);
    RUN_TEST(TestCountRelevanse);
}

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
