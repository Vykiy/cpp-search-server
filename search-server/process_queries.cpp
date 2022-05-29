#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries) {

    vector<vector<Document>> result(queries.size());
    std::transform(std::execution::par,
              queries.begin(), queries.end(),
              result.begin(),
              [&search_server] (const string& queries) {
              return search_server.FindTopDocuments(queries); }
    );

    return result;
}


vector<Document> ProcessQueriesJoined(const SearchServer& search_server,
                                      const std::vector<std::string>& queries) {
    vector<Document> result;
    vector<vector<Document>> tmp(ProcessQueries(search_server, queries));

    for (const auto& document : tmp) {
        copy(std::execution::par, document.begin(), document.end(), back_inserter(result));
    }

    return result;
}
