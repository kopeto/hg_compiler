#include "grid_word.h"
#include "cell.h"

GridWord::GridWord(GridWordDirection dir, unsigned int r, unsigned int c, unsigned int len) 
    : direction(dir), length(len), _str(len, '_'), _starting_row(r), _starting_col(c) {}

void GridWord::addCell(Cell* cell) {
    cells.push_back(cell);
}

void GridWord::setWord(const std::string& str) {
    
    if(str.size() != cells.size()) {
        std::cout << "Error: String length (" << str.size() << ") does not match the number of cells (" << cells.size() << ") in the grid word." << std::endl;
        throw std::invalid_argument("String length must match the number of cells in the grid word");
    }
    
    // Update the value of each cell in the grid word based on the provided string
    for (size_t i = 0; i < cells.size() && i < str.size(); ++i) {
        cells[i]->value = str[i];
    }

    // Update the internal string representation of the grid word
    _str = str;
}

const std::string& GridWord::getWord() {
    // Construct the current string based on the values of the cells in the grid word
    for (size_t i = 0; i < cells.size(); ++i) {
        _str[i] = cells[i]->value;
    }
    return _str;
}

const std::string& GridWord::getString() const {
    return _str;
}

const std::pair<int,int> GridWord::getPosition() const {
    return {_starting_row, _starting_col};
}

void GridWord::set() {
    _is_word_set = true;
}

void GridWord::unset() {
    _is_word_set = false;
}

bool GridWord::isSet() const {
    return _is_word_set;
}