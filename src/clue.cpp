#include "clue.h"

Clue::Clue(const std::string& clueText, const std::string& answer) : _clueText(clueText), _answer(answer) {}

const std::string& Clue::getClueText() const {
    return _clueText;
}

const std::string& Clue::getAnswer() const {
    return _answer;
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
