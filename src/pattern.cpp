#include "pattern.h"

#include <algorithm>

Pattern::Pattern(const std::string& s) : str(s) {
    std::transform(s.begin(), s.end(), str.begin(), ::toupper);
    if (!_validatePattern(str)) {
        throw std::invalid_argument("Invalid pattern: " + str);
    }
}

bool Pattern::_validatePattern(const std::string& pattern) {
    for (char c : pattern) {
        if (c != '_' && !std::isalpha(c)) {
            std::cout << "Error: Invalid character '" << c << "' found in pattern " << pattern << "." << std::endl;
            return false; // Invalid character found
        }
    }
    return true; // All characters are valid
}

bool Pattern::isZeroPattern() const {
    return std::all_of(str.begin(), str.end(), [](char c) { return c == '_'; });
}

std::vector<std::string> Pattern::getPatternIndexes() const {
    std::vector<std::string> indexes;
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] != '_') {
            char index[5];
            snprintf(index, 5, "%c_%zu", std::toupper(str[i]), i % 32);
            indexes.emplace_back(index);
        }
    }
    return indexes;
}

void Pattern::FilterWordsByPattern(std::vector<const Word*>& words, const Pattern& pattern) {
    words.erase(std::remove_if(words.begin(), words.end(),
                               [&pattern](const Word* word) {
                                   if (word->size() != pattern.size()) {
                                       return true; // Remove words of different length
                                   }
                                   for (std::size_t i = 0; i < pattern.size(); ++i) {
                                       if (pattern.str[i] != '_' && pattern.str[i] != word->str[i]) {
                                           return true; // Remove words that don't match the pattern
                                       }
                                   }
                                   return false; // Keep words that match the pattern
                               }),
                words.end());
}
