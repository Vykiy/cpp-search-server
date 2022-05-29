#include "string_processing.h"

using namespace std;


vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> words;
    while (true) {
        auto space = text.find(' ', 0);
        words.push_back(text.substr(0, space));
        if (space == text.npos) {
            break;
        } else {
            text.remove_prefix(space + 1);
        }
    }
    return words;
}

/*
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}
*/
