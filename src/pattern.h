#pragma once

#include "word.h"

#include <iostream>
#include <string>
#include <vector>

struct Pattern {
    static void FilterWordsByPattern(std::vector<const Word*>& words, const Pattern& pattern);

    Pattern() = default;
    Pattern(const std::string& s);
    ~Pattern() = default;

    std::vector<std::string> getPatternIndexes() const;
    bool                     isZeroPattern() const;
    inline std::size_t       size() const { return str.size(); }

public:
    std::string str{""};

private:
    static bool _validatePattern(const std::string& pattern);
};