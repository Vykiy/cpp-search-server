#pragma once

#include "search_server.h"
#include<string>
#include <vector>

void AddDocument (SearchServer &search_server, const int& id, const string& documents,
                  DocumentStatus status, const vector<int> &rating);

void FindTopDocuments(const SearchServer& search_server, const string& raw_query);

void MatchDocuments(const SearchServer& search_server, const string& query);