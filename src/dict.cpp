#include "dict.h"

#include "timer/profiler.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

// ── helpers ───────────────────────────────────────────────────

// Parse one line from a dictionary file.
// Supported formats:
//   "WORD"         → plain word  (score = 0)
//   "WORD;SCORE"   → word with numeric score
// Returns {uppercased_word, score}, or {"", 0} if the line should be skipped.
static std::pair<std::string, int> parseDictLine(const std::string& line) {
    if (line.empty())
        return {};

    auto sep = line.find(';');

    // Word part: everything before ';'
    std::string word = line.substr(0, sep);

    // Score part: everything after ';', if present
    int score = 0;
    if (sep != std::string::npos) {
        try {
            score = std::stoi(line.substr(sep + 1));
        } catch (...) {
            score = 0;
        }
    }

    // Trim trailing whitespace/CR from word
    while (!word.empty() && (word.back() == ' ' || word.back() == '\r' || word.back() == '\n'))
        word.pop_back();

    if (word.empty())
        return {};

    // Validate: only alpha characters allowed
    for (unsigned char c : word)
        if (!std::isalpha(c))
            return {};

    std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) { return std::toupper(c); });
    return {word, score};
}

// ── Dict ──────────────────────────────────────────────────────

Dict::Dict() {
    std::ifstream file(HG_DEFAULT_DICTIONARY_PATH);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << HG_DEFAULT_DICTIONARY_PATH << std::endl;
        return;
    }

    _wordsByLength.resize(100);

    std::string line;
    while (std::getline(file, line)) {
        auto [word, score] = parseDictLine(line);
        if (!word.empty())
            _words.emplace_back(word, score);
    }

    for (const auto& word : _words) {
        if (word.size() > 0 && word.size() <= _wordsByLength.size()) {
            _wordsByLength[word.size() - 1].push_back(&word);
        }
    }

    for (auto& vec : _wordsByLength) {
        std::sort(vec.begin(), vec.end(), [](const Word* a, const Word* b) { return a->str < b->str; });
    }
}

Dict::~Dict() = default;

Dict::Dict(const std::string& filepath) {
    load(filepath);
}

void Dict::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Could not open dictionary file: " + filename);

    _words.clear();
    _wordsByLength.clear();
    _wordsByLength.resize(100);

    std::string line;
    while (std::getline(file, line)) {
        auto [word, score] = parseDictLine(line);
        if (!word.empty())
            _words.emplace_back(word, score);
    }

    for (const auto& w : _words) {
        if (w.size() > 0 && w.size() <= _wordsByLength.size())
            _wordsByLength[w.size() - 1].push_back(&w);
    }

    for (auto& vec : _wordsByLength)
        std::sort(vec.begin(), vec.end(), [](const Word* a, const Word* b) { return a->str < b->str; });
}

std::vector<const Word*> Dict::getWordsOfLength(std::size_t length) const {
    if (length < _wordsByLength.size() && length > 0) {
        return _wordsByLength[length - 1];
    }
    return {};
}

std::vector<const Word*> Dict::getWordsByPattern(const Pattern& pattern) const {
    // Pattern example: "_A_E" (matches words like "CAVE", "WAVE", etc.)

    /*
    1- Get list of words with the same length as the pattern (_wordsByLength list).
    2- For each word, check if it matches the pattern:
    3- For each character in the pattern:
     - If the character is '_', it can match any character in the word.
     - If the character is a letter, it must match the corresponding character in the word.
    4- If a word matches the pattern, add it to the result list.
    */

    HG_PROFILE_SCOPE("Dict::getWordsByPattern::" + std::to_string(pattern.str.size()));

    std::vector<const Word*> result;
    std::size_t              patternLength = pattern.size();

    if (patternLength >= _wordsByLength.size()) {
        return result; // No words of this length
    }

    for (auto word : _wordsByLength[patternLength - 1]) {
        bool matches = true;
        for (std::size_t i = 0; i < patternLength; ++i) {
            if (pattern.str[i] != '_' && pattern.str[i] != word->str[i]) {
                matches = false;
                break;
            }
        }
        if (matches) {
            result.push_back(word);
        }
    }

    return result;
}

std::vector<const Word*> Dict::getWordsByPattern_indexed(const Pattern& pattern) const {
    HG_PROFILE_SCOPE("Dict::getWordsByPattern_indexed::" + pattern.str);
    Pattern p(pattern);
    if (p.isZeroPattern()) {
        return getWordsOfLength(pattern.size());
    } else {
        return _indexedWords.getWordsByPattern(pattern);
    }
}

Dict::IndexedWords::IndexedWords(const Dict& dict) {
    buildIndex(dict);
}

void Dict::IndexedWords::buildIndex(const Dict& dict) {
    // avoid warnings
    (void)dict;
    // HG_PROFILE_FUNCTION();
    // for (unsigned int i = 0; i < Dict::MAX_WORD_LENGTH; ++i)
    // {
    //     for (const char c : "ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    //     {
    //         std::string index = std::string(1, c) + "_" + std::to_string(i);

    //         for (unsigned int j = i; j < Dict::MAX_WORD_LENGTH; ++j)
    //         {
    //             std::vector<Word> matchingWords;
    //             for (const auto &word : dict._wordsByLength[j])
    //             {
    //                 if (std::toupper(word.str[i]) == c)
    //                 {
    //                     matchingWords.push_back(word);
    //                 }
    //             }
    //             _indexed[index].insert(_indexed[index].end(), matchingWords.begin(), matchingWords.end());
    //         }
    //     }
    // }
}

std::vector<const Word*> Dict::IndexedWords::getWordsByPattern(const Pattern& pattern) const {
    std::vector<const Word*> result;

    std::vector<std::string> patternIndexes = pattern.getPatternIndexes();
    std::size_t              patternLength  = pattern.size();

    if (patternIndexes.empty()) {
        return result; // No specific character constraints, return empty result
    }

    result = _indexed.at(patternIndexes[0]); // Start with the first pattern index
    // match legth of the pattern with the length of the words in the index
    result.erase(std::remove_if(result.begin(), result.end(),
                                [patternLength](const Word* word) { return word->size() != patternLength; }),
                 result.end());

    for (unsigned int i = 1; i < patternIndexes.size(); ++i) {
        const auto& index = patternIndexes[i];
        auto        words = _indexed.at(index);
        words.erase(std::remove_if(words.begin(), words.end(),
                                   [patternLength](const Word* word) { return word->size() != patternLength; }),
                    words.end());

        // Intersect the current result with the new set of words
        std::vector<const Word*> intersection;
        std::set_intersection(result.begin(), result.end(), words.begin(), words.end(),
                              std::back_inserter(intersection),
                              [](const Word* a, const Word* b) { return a->str < b->str; });
        result = std::move(intersection);
    }

    return result;
}

bool Dict::contains(const std::string& word) const {
    std::size_t length = word.size();
    if (length == 0 || length > _wordsByLength.size()) {
        return false;
    }

    const auto& wordsOfLength = _wordsByLength[length - 1];
    return std::find_if(wordsOfLength.begin(), wordsOfLength.end(),
                        [&word](const Word* w) { return w->str == word; }) != wordsOfLength.end();
}

bool Dict::contains(const Word& word) const {
    return contains(word.str);
}
