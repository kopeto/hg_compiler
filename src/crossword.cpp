#include "crossword.h"

#include <iostream>

Crossword::Crossword() : _grid(HG_DEFAULT_GRID_PATH) {
    _width  = _grid.getCols();
    _height = _grid.getRows();
}

Crossword::Crossword(const std::string& gridFilePath) : _grid(gridFilePath) {
    _width  = _grid.getCols();
    _height = _grid.getRows();
}

Crossword::Crossword(const std::vector<std::string>& lines) : _grid(lines) {
    _width  = _grid.getCols();
    _height = _grid.getRows();
}

void Crossword::setCell(int x, int y, char value) {
    if (x >= 0 && x < _width && y >= 0 && y < _height) {
        _grid.setCell(y, x, value);
    }
}

void Crossword::printGrid() const {
    _grid.print();
}

const std::vector<GridWord>& Crossword::getGridAcrossWords() const {
    return _grid.getAcrossWords();
}

const std::vector<GridWord>& Crossword::getGridDownWords() const {
    return _grid.getDownWords();
}

const std::vector<GridWord>& Crossword::getGridWords() const {
    _allWords.clear();
    const auto& across = _grid.getAcrossWords();
    const auto& down   = _grid.getDownWords();
    _allWords.reserve(across.size() + down.size());
    _allWords.insert(_allWords.end(), across.begin(), across.end());
    _allWords.insert(_allWords.end(), down.begin(), down.end());
    return _allWords;
}

Grid& Crossword::getGrid() {
    return _grid;
}
