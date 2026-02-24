#pragma once

#include "pattern.h"
#include "word.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

struct Dict {
    // Some constants
    static constexpr std::size_t MAX_WORD_LENGTH = 30; // Arbitrary max length for words

    struct IndexedWords {
        IndexedWords() = default;
        IndexedWords(const Dict& dict);
        ~IndexedWords() = default;

        // Builds the index from the given dictionary
        void buildIndex(const Dict& dict);

        std::vector<const Word*> getWordsByPattern(const Pattern& pattern) const;

    private:
        std::unordered_map<std::string, std::vector<const Word*>> _indexed;
    };

    Dict();
    explicit Dict(const std::string& filepath);
    ~Dict();

    // load the dictionary from a file, where each line contains a single word
    void load(const std::string& filename);

    std::vector<const Word*> getWordsByPattern(const Pattern& pattern) const;
    std::vector<const Word*> getWordsByPattern_indexed(const Pattern& pattern) const;
    std::vector<const Word*> getWordsOfLength(std::size_t length) const;

    bool contains(const std::string& word) const;
    bool contains(const Word& word) const;

private:
    std::vector<Word>                     _words;
    std::vector<std::vector<const Word*>> _wordsByLength; // Optional: Pre-categorize words by length for faster access
    IndexedWords                          _indexedWords;  // For pattern-based retrieval
};