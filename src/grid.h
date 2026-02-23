#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "dict.h"
#include "cell.h"
#include "grid_word.h"

struct Grid
{
    /**
     * Static functions that are not specific to a particular instance of the Grid class, but are useful for the overall functionality of the grid filling algorithm. These functions can be called without needing an instance of the Grid class, and they operate on the data passed to them as arguments.
     */
    // get next grid word to fill
    // static GridWord* getNextGridWordToFill(const std::vector<GridWord*>& _to_fill, const std::vector<GridWord*>& _filled);
    static GridWord* getNextGridWordToFill(const std::vector<GridWord*>& _to_fill, const std::vector<GridWord*>& _filled, const Dict& dict);

    // Check two grid words cross each other
    static bool crossing(const GridWord* word1, const GridWord* word2);

    Grid() = delete;
    Grid(const std::string& filename);
    Grid(const std::vector<std::string>& lines);  // construct from in-memory lines
    ~Grid();
    Grid(const Grid& other);
    Grid& operator=(const Grid& other);

    int     getRows() const { return _rows; }
    int     getCols() const { return _cols; }

    void    setCell(int r, int c, int value);
    void    fixCell(int r, int c, char letter);   // set letter + mark fixed
    void    unfixCell(int r, int c);              // clear fixed flag
    bool    isFixed(int r, int c) const;
    int     getValue(int r, int c) const;
    void    print() const;

    GridWord* getGridWordAt(unsigned int r, unsigned int c, GridWordDirection direction) const;

    // Get all down/across words in the grid
    const std::vector<GridWord>& getAcrossWords() const noexcept { return _acrossWords; }
    const std::vector<GridWord>& getDownWords() const noexcept { return _downWords; }
    

    /**
     * Fill the grid with words from the dictionary. This function will use a backtracking algorithm to fill the grid, starting from the first grid word and trying to fill it with a word from the dictionary that matches the current pattern of the grid word. If we successfully fill the grid, we return true. If we exhaust all possibilities and cannot fill the grid, we return false.
     */
    bool solve(const Dict& dict);

    /**
     * Returns true if every FILLABLE cell has a letter (not '_').
     */
    bool isSolved() const;

    /**
     * Resets all FILLABLE cells to '_' and clears GridWord state,
     * so the grid is ready for a fresh solve.
     */
    void reset();

    /**
     * Fill the grid starting from a specific grid word, using a backtracking algorithm.
     * The algorithm works as follows:
     * 1. Get the current pattern of the grid word (the string formed by the cells in the grid word, where empty cells are represented by '_').
     * 2. Get the list of possible words from the dictionary that match the current pattern.
     * 3. Shuffle the list of possible words to ensure that we explore different possibilities in different runs of the algorithm.
     * 4. For each possible word, fill the grid word with the word and then recursively call the fillFrom function to fill the next grid word. If the recursive call returns true, it means that we have successfully filled the grid and we can return true. If the recursive call returns false, it means that we need to backtrack and try the next possible word.
     * 5. If we have tried all possible words and none of them fit, we need to backtrack to the previous grid word and try a different word for it. This is the essence of the backtracking algorithm.
     * 
     * It keeps track of the current state of the grid and the grid words that have been filled so far, so that it can easily backtrack when needed. The function returns true if the grid was successfully filled, and false if it was not possible to fill the grid with the given dictionary.
     */
    bool solve( GridWord* grid_word,  std::vector<GridWord*>& _to_fill, std::vector<GridWord*>& _filled, const Dict& dict);

    /**
     * Get a vector of current word's crossing words (words that intersect with it).
     */
    std::vector<GridWord*> getCrossingWords(const GridWord* word) const;


private:
    void initFromLines(const std::vector<std::string>& lines);

    int _rows;
    int _cols;
    std::vector<std::vector<Cell>> _cells;
    std::vector<GridWord> _acrossWords; // 1D vector to store all across words
    std::vector<GridWord> _downWords;   // 1D vector to store all down words
};