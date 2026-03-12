#pragma once

#include <string>

struct GridWord; // Forward declaration to avoid circular dependency
struct Cell;     // Forward declaration to avoid circular dependency

struct Clue {
    Clue() = default;
    Clue(const std::string& clueText, const std::string& answer);
    ~Clue() = default;

    const std::string& getClueText() const;
    const std::string& getAnswer() const;

    GridWord* getGridWord() const;
    void      setGridWord(GridWord* gridWord);

    Cell* getCell() const;
    void  setCell(Cell* cell);

private:
    GridWord*   _gridWord{nullptr}; // Pointer to the associated GridWord, if needed
    Cell*       _cell{nullptr};     // Pointer to the associated Cell, if needed
    std::string _clueText{};
    std::string _answer{};
};