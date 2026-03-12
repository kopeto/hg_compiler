#include "clue.h"

Clue::Clue(const std::string& clueText) : _clueText(clueText) {}

const std::string& Clue::getClueText() const {
    return _clueText;
}

void Clue::setClueText(const std::string& text) {
    _clueText = text;
}

GridWord* Clue::getGridWord() const {
    return _gridWord;
}

void Clue::setGridWord(GridWord* gridWord) {
    _gridWord = gridWord;
}

Cell* Clue::getCell() const {
    return _cell;
}

void Clue::setCell(Cell* cell) {
    _cell = cell;
}
