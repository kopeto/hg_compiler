#pragma once

#include <string>

struct Word
{
    Word() = default;
    Word(const std::string &s) : str(s), _size(str.size()) {}

    bool operator==(const Word &other) const
    {
        return str == other.str;
    }

    std::size_t size() const
    {
        return _size;
    }

public:
    std::string str{""};

private:
    std::size_t _size = 0;
};