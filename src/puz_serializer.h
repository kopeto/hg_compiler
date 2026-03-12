#pragma once

#include "grid.h"

#include <QString>
#include <string>
#include <vector>

// Data extracted from a .puz file
struct PuzData {
    int  rows = 0;
    int  cols = 0;

    // One string per row, length == cols.
    // '.' = black cell, letter = filled cell, '-' = empty white cell.
    std::vector<std::string> solution;

    std::string              title;
    std::string              author;
    std::string              copyright;
    std::vector<std::string> clues; // in .puz reading order
    std::string              notes;

    QString errorMessage; // non-empty on parse failure
};

class PuzSerializer {
public:
    // Export grid to a .puz file.
    // defaultClue is used for every clue since we have no clue data yet.
    // Returns an empty string on success, or an error message on failure.
    static QString exportToFile(const Grid&  grid,
                                const QString& path,
                                const std::string& defaultClue = "Ez dago pistarik");

    // Import a .puz file.  Check PuzData::errorMessage for errors.
    static PuzData importFromFile(const QString& path);

    // Convert PuzData into grid-file lines (suitable for Grid(lines) constructor).
    // White cells with a letter keep the letter; empty white cells become '.';
    // black cells become '#'.
    static std::vector<std::string> toGridLines(const PuzData& data);
};
