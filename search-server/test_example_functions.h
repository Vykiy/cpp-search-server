#pragma once

#include <iostream>
#include <execution>
#include <stdexcept>
#include <deque>
#include <string>
#include <vector>

#include "search_server.h"
#include "document.h"



void AddDocument(SearchServer& search_server, int document_id, std::string_view document, DocumentStatus status,
                 const std::vector<int>& ratings);

void FindTopDocuments(const SearchServer& search_server, std::string_view raw_query);

void MatchDocuments(const SearchServer& search_server, std::string_view query);


