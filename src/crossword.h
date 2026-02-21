#pragma once

#include <iostream>

#include "grid.h"

struct Crossword {

    Crossword();

    Crossword(const std::string& gridFilePath);
    
    void setCell(int x, int y, char value);

    void printGrid() const;

    const std::vector<GridWord>& getGridAcrossWords() const;

    const std::vector<GridWord>& getGridDownWords() const;

    Grid& getGrid() ;

private:
    int _width;
    int _height;
    Grid _grid;
};