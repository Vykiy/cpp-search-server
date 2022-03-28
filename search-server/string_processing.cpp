#include "string_processing.h"

#include <algorithm>
#include <stdexcept>

using namespace std::string_literals;

bool IsValidText(const std::string& text) {
    if (!none_of(text.begin(), text.end(), [](char c) { return c >= '\0' && c < ' '; }))
    {
        throw std::invalid_argument("Недопустимые символы"s);
    }
    return true;
}

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
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
