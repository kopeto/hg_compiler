#pragma once

#include "word.h"

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

struct Cell;

enum class GridWordDirection { ACROSS, DOWN };

struct GridWord {

    GridWord(GridWordDirection dir, unsigned int r, unsigned int c, unsigned int len);

    void addCell(Cell* cell);

    // Set the current string formed by the cells in this grid word
    void setWord(const std::string& str);

    // Get the current string formed by the cells in this grid word
    const std::string& getWord();

    // Get the current string formed by the cells in this grid word
    const std::string& getString() const;

    const std::pair<int, int> getPosition() const;

    /**
     * set or unset word
     */
    void set();
    void unset();

    /**
     * Check if is set
     */
    bool isSet() const;

    /**
     * Returns true if every cell in this word is fixed by the user.
     * Such words are skipped by the solver (no dictionary lookup needed).
     */
    bool isFullyFixed() const;

public:
    std::vector<Cell*> cells{}; // Pointers to the cells that make up this grid word
    GridWordDirection  direction;
    size_t             length;
    std::vector<const Word*>
        possible_words{}; // Possible words that can fit in this grid word based on the current state of the grid

private:
    std::string _str;                // The current string formed by the cells in this grid word
    int         _starting_row;       // Starting row of the grid word
    int         _starting_col;       // Starting column of the grid word
    bool        _is_word_set{false}; // Flag to indicate if the current string is set or not
};