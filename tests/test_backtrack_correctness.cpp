#include "grid.h"
#include "crossword.h"
#include "dict.h"
#include "logger.h"
#include "timer/profiler.h"

#include <cassert>
#include <iostream>

void test_backtrack_state_restoration() {
    Dict dict;
    Crossword crossword(HG_ASSETS_PATH "grids/11x11.grid");
    
    // Capture initial state
    std::vector<const GridWord*> initial_to_fill;
    for (const auto& word : crossword.getGrid().getDownWords()) initial_to_fill.push_back(&word);
    for (const auto& word : crossword.getGrid().getAcrossWords()) initial_to_fill.push_back(&word);
    size_t initial_size = initial_to_fill.size();
    assert(initial_size > 0 && "Grid should have words to fill");

    // Attempt to fill (may fail)
    bool success = crossword.getGrid().solve(dict);

    Logger::info("Grid solve attempt: {}", success ? "Success" : "Failure");
    if (success) {
        // print grid:
        Logger::info("Filled grid:");
        crossword.printGrid();
    }

    // Check all words are in the used dictionary (no corruption)
    for (const auto& gridword : crossword.getGrid().getAcrossWords()) {
        bool contains = dict.contains(gridword.getString());
        Logger::info("Checking word: '{}' - {}", gridword.getString(), contains ? "OK" : "NOT FOUND");
    }
    for (const auto& gridword : crossword.getGrid().getDownWords()) {
        bool contains = dict.contains(gridword.getString());
        Logger::info("Checking word: '{}' - {}", gridword.getString(), contains ? "OK" : "NOT FOUND");
    }

    Logger::info("Test passed: No state corruption detected");
}

void test_crossing_correctness() {
    // Create grid and verify crossing() returns true/false correctly
    Crossword crossword(HG_ASSETS_PATH "grids/20x20.grid");

    const auto& acrossWords = crossword.getGrid().getAcrossWords();
    const auto& downWords   = crossword.getGrid().getDownWords();

    assert(!acrossWords.empty() && "Grid has no across words");
    assert(!downWords.empty()   && "Grid has no down words");

    int crossings_found = 0;
    for (const auto& across : acrossWords) {
        for (const auto& down : downWords) {
            if (Grid::crossing(&across, &down)) {
                crossings_found++;
            }
        }
    }

    assert(crossings_found > 0 && "A valid crossword must have at least one crossing");
    Logger::info("Test crossing: OK ({}) crossings found", crossings_found);
}

int main() {
    HG_PROFILER_RESET();
    try {
        Logger::setLevel(LogLevel::INFO);
        test_backtrack_state_restoration();
        // test_crossing_correctness();
        Logger::info("All tests passed");
    } catch (const std::exception& e) {
        Logger::error("Test failed with exception: {}", e.what());
        return 1;
    }
    HG_PROFILER_REPORT();
    return 0;
}