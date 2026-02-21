#include <iostream>
#include <filesystem>

#include "crossword.h"
#include "dict.h"
#include "timer/profiler.h"

int main(int argc, char **argv)
{
    HG_PROFILER_RESET();

    std::string pattern = "_A_E";

    if (argc > 1)
        pattern = argv[1];

    Crossword crossword;
    crossword.printGrid();

    Dict dict;

    auto words = dict.getWordsByPattern(pattern);
    std::cout << "Matching list size: " << words.size() << std::endl;
    // first 10 words
    for (size_t i = 0; i < std::min(words.size(), size_t(10)); ++i)
    {
        std::cout << words[i]->str << std::endl;
    }

    HG_PROFILER_REPORT();
    return 0;
}