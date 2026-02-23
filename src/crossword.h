#pragma once

#include <iostream>
#include <vector>
#include <string>

#include "grid.h"

struct Crossword {

    Crossword();
    Crossword(const std::string& gridFilePath);
    Crossword(const std::vector<std::string>& lines);
    
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