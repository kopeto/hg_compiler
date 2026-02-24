#pragma once

#include "grid_word.h"

#include <iostream>

struct Cell {
    enum CellType { FILLABLE, BLACK };

    Cell() = default;
    Cell(char c) {
        if (c == '#') {
            value = '#';
            type  = CellType::BLACK;
        } else {
            if (c == '.') {
                value = '_';
            } else {
                value = c;
            }
            type = CellType::FILLABLE;
        }
    }

    // overload the assignment operator with a char
    Cell& operator=(char c) {
        if (c == '#') {
            value = '#';
            type  = CellType::BLACK;
        } else {
            if (c == '.') {
                value = '_';
            } else {
                value = c;
            }
            type = CellType::FILLABLE;
        }
        return *this;
    }

    bool operator==(const Cell& c) const { return value == c.value && type == c.type; }

    // use it with iostream
    friend std::ostream& operator<<(std::ostream& os, const Cell& cell) {
        os << cell.value;
        return os;
    }

public:
    // The character in the cell, or '.' if it's empty
    char value;
    // Pointer to the horizontal grid word this cell belongs to
    GridWord* horizontal_word = nullptr;
    // Pointer to the vertical grid word this cell belongs to
    GridWord* vertical_word = nullptr;
    // The type of the cell (fillable or black)
    CellType type;
    // When true, the solver will not overwrite this cell and reset() won't clear it
    bool fixed = false;
};