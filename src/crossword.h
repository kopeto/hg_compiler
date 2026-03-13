#pragma once

#include "grid.h"

#include <iostream>
#include <string>
#include <vector>

struct Crossword {

    Crossword();
    Crossword(const std::string& gridFilePath);
    Crossword(const std::vector<std::string>& lines);

    void setCell(int x, int y, char value);

    void printGrid() const;

    const std::vector<GridWord>& getGridAcrossWords() const;

    const std::vector<GridWord>& getGridDownWords() const;

    const std::vector<GridWord>& getGridWords() const;

    Grid& getGrid();

    // ── Metadata ──────────────────────────────────────────
    std::string title;
    std::string author;
    std::string copyright = "\u00a9 2026 HitzGurutzatuak";

private:
    int  _width;
    int  _height;
    Grid _grid;
    mutable std::vector<GridWord> _allWords;
};