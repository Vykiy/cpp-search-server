#pragma once

#include <string>
#include <vector>
#include <set>

using std::set;
using std::string;
using std::vector;

vector<string> SplitIntoWords (const string &text);

template<typename StringContainer>
set<string> MakeUniqueNonEmptyStrings (const StringContainer &strings) {
    set<string> non_empty_strings;
    for (const string &str: strings) {
        if (! str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}