#include "grid.h"

#include "logger.h"
#include "timer/profiler.h"

#include <algorithm>
#include <fstream>
#include <random>
#include <set>
#include <stdexcept>

Grid::Grid(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("Could not open file: " + filename);

        std::vector<std::string> lines;
        std::string              line;
        while (std::getline(file, line))
            lines.push_back(line);

        initFromLines(lines);
        Logger::debug("Grid loaded from file: {}", filename);
    } catch (const std::exception& e) {
        std::cerr << "Error loading grid: " << e.what() << std::endl;
        throw;
    }
}

Grid::Grid(const std::vector<std::string>& lines) {
    initFromLines(lines);
}

void Grid::initFromLines(const std::vector<std::string>& lines) {
    if (lines.empty())
        throw std::runtime_error("Grid has no lines");

    int cols = static_cast<int>(lines[0].size());
    for (const auto& line : lines) {
        if (static_cast<int>(line.size()) != cols)
            throw std::runtime_error("Inconsistent line length in grid");

        std::vector<Cell> row;
        for (char c : line) {
            char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            if (c != '#' && c != '.' && !(upper >= 'A' && upper <= 'Z'))
                throw std::runtime_error("Invalid character in grid: " + std::string(1, c));
            row.emplace_back(c);
        }
        _cells.push_back(row);
    }
    _rows = static_cast<int>(_cells.size());
    _cols = cols;

    _acrossWords.reserve(_rows);
    _downWords.reserve(_cols);

    // ACROSS words — pass 1: only create GridWords
    std::vector<Cell*> current_word_cells;
    int                current_word_start_c = -1;

    for (int r = 0; r < _rows; r++) {
        for (int c = 0; c < _cols; c++) {
            if (_cells[r][c].type == Cell::CellType::FILLABLE) {
                current_word_cells.push_back(&_cells[r][c]);
                if (current_word_start_c == -1)
                    current_word_start_c = c;
            } else if (_cells[r][c].type == Cell::CellType::BLACK) {
                if (current_word_cells.size() >= 2)
                    _acrossWords.emplace_back(GridWordDirection::ACROSS, r, current_word_start_c,
                                              current_word_cells.size());
                current_word_cells.clear();
                current_word_start_c = -1;
            }
        }
        if (current_word_cells.size() >= 2)
            _acrossWords.emplace_back(GridWordDirection::ACROSS, r, current_word_start_c, current_word_cells.size());
        current_word_cells.clear();
        current_word_start_c = -1;
    }

    // DOWN words — pass 1
    int current_word_start_r = -1;
    for (int c = 0; c < _cols; c++) {
        for (int r = 0; r < _rows; r++) {
            if (_cells[r][c].type == Cell::CellType::FILLABLE) {
                current_word_cells.push_back(&_cells[r][c]);
                if (current_word_start_r == -1)
                    current_word_start_r = r;
            } else if (_cells[r][c].type == Cell::CellType::BLACK) {
                if (current_word_cells.size() >= 2)
                    _downWords.emplace_back(GridWordDirection::DOWN, current_word_start_r, c,
                                            current_word_cells.size());
                current_word_cells.clear();
                current_word_start_r = -1;
            }
        }
        if (current_word_cells.size() >= 2)
            _downWords.emplace_back(GridWordDirection::DOWN, current_word_start_r, c, current_word_cells.size());
        current_word_cells.clear();
        current_word_start_r = -1;
    }

    // PASS 2: link cells to GridWords (vectors are stable now)
    for (auto& word : _acrossWords) {
        auto [row, col] = word.getPosition();
        for (size_t i = 0; i < word.length; i++) {
            Cell* cell            = &_cells[row][col + i];
            cell->horizontal_word = &word;
            word.addCell(cell);
        }
    }
    for (auto& word : _downWords) {
        auto [row, col] = word.getPosition();
        for (size_t i = 0; i < word.length; i++) {
            Cell* cell          = &_cells[row + i][col];
            cell->vertical_word = &word;
            word.addCell(cell);
        }
    }

    Logger::debug("Grid built: {}x{}, {} across, {} down", _rows, _cols, _acrossWords.size(), _downWords.size());
}

Grid::~Grid() {}
Grid::Grid(const Grid& other) : _rows(other._rows), _cols(other._cols), _cells(other._cells) {}

Grid& Grid::operator=(const Grid& other) {
    if (this != &other) {
        _rows  = other._rows;
        _cols  = other._cols;
        _cells = other._cells; // std::vector will handle the copying
    }
    return *this;
}

void Grid::setCell(int r, int c, int value) {
    if (r >= 0 && r < _rows && c >= 0 && c < _cols)
        _cells[r][c] = static_cast<char>(value);
}

void Grid::fixCell(int r, int c, char letter) {
    if (r >= 0 && r < _rows && c >= 0 && c < _cols && _cells[r][c].type == Cell::CellType::FILLABLE) {
        _cells[r][c].value = static_cast<char>(std::toupper((unsigned char)letter));
        _cells[r][c].fixed = true;
    }
}

void Grid::unfixCell(int r, int c) {
    if (r >= 0 && r < _rows && c >= 0 && c < _cols && _cells[r][c].type == Cell::CellType::FILLABLE) {
        _cells[r][c].fixed = false;
        _cells[r][c].value = '_'; // clear the letter too
    }
}

bool Grid::isFixed(int r, int c) const {
    if (r >= 0 && r < _rows && c >= 0 && c < _cols)
        return _cells[r][c].fixed;
    return false;
}

bool Grid::cellHasWord(int r, int c) const {
    if (r < 0 || r >= _rows || c < 0 || c >= _cols)
        return false;
    const Cell& cell = _cells[r][c];
    if (cell.type != Cell::CellType::FILLABLE)
        return false;
    return cell.horizontal_word != nullptr || cell.vertical_word != nullptr;
}

int Grid::getValue(int r, int c) const {
    if (r >= 0 && r < _rows && c >= 0 && c < _cols) {
        return _cells[r][c].value;
    }
    throw std::out_of_range("Cell coordinates out of range");
}

void Grid::print() const {
    for (const auto& row : _cells) {
        std::string line{};
        for (const auto& cell : row) {
            line += static_cast<char>(std::toupper(static_cast<unsigned char>(cell.value)));
        }
        Logger::info("{}", line);
    }
}

bool Grid::isSolved() const {
    for (const auto& row : _cells)
        for (const auto& cell : row)
            if (cell.type == Cell::CellType::FILLABLE && cell.value == '_')
                return false;
    return true;
}

void Grid::reset() {
    // Reset every fillable, non-fixed cell to '_'
    for (auto& row : _cells)
        for (auto& cell : row)
            if (cell.type == Cell::CellType::FILLABLE && !cell.fixed)
                cell.value = '_';

    // Reset GridWord internal state so the solver starts fresh.
    // Words that are already fully satisfied by fixed cells are left alone
    // (the solver will detect them as already filled).
    for (auto& word : _acrossWords) {
        word.possible_words.clear();
        word.unset();
        // _str re-synced from cell values (fixed cells keep their letter)
        word.getWord();
    }
    for (auto& word : _downWords) {
        word.possible_words.clear();
        word.unset();
        word.getWord();
    }
}

GridWord* Grid::getGridWordAt(unsigned int r, unsigned int c, GridWordDirection direction) const {
    // cast a unsigned int para evitar warning
    if (r >= static_cast<unsigned int>(_rows) || c >= static_cast<unsigned int>(_cols) ||
        _cells[r][c].type != Cell::CellType::FILLABLE) {
        throw std::out_of_range("Cell coordinates out of range or cell is not fillable");
    }

    if (direction == GridWordDirection::ACROSS) {
        return _cells[r][c].horizontal_word;
    } else if (direction == GridWordDirection::DOWN) {
        return _cells[r][c].vertical_word;
    } else {
        throw std::invalid_argument("Invalid grid word direction");
    }
    return nullptr; // Should never reach here
}

bool Grid::solve(const Dict& dict, std::atomic<bool>* cancelFlag) {
    std::vector<GridWord*> _to_fill;
    std::vector<GridWord*> _filled;

    for (auto& word : _acrossWords)
        _to_fill.push_back(&word);
    for (auto& word : _downWords)
        _to_fill.push_back(&word);

    // Words that are fully fixed by the user need no dictionary lookup —
    // move them directly to _filled and mark them as set.
    auto it = std::remove_if(_to_fill.begin(), _to_fill.end(), [](GridWord* w) { return w->isFullyFixed(); });
    for (auto jt = it; jt != _to_fill.end(); ++jt) {
        (*jt)->set();
        _filled.push_back(*jt);
        Logger::debug("Skipping fully-fixed word at {},{} : {}", (*jt)->getPosition().first,
                      (*jt)->getPosition().second, (*jt)->getWord());
    }
    _to_fill.erase(it, _to_fill.end());

    if (_to_fill.empty())
        return true;

    GridWord* firstWord = getNextGridWordToFill(_to_fill, _filled, dict);
    _to_fill.erase(std::remove(_to_fill.begin(), _to_fill.end(), firstWord), _to_fill.end());

    return solve(firstWord, _to_fill, _filled, dict, cancelFlag);
}

bool Grid::solve(GridWord* current_word_to_fill, std::vector<GridWord*>& _to_fill, std::vector<GridWord*>& _filled,
                 const Dict& dict, std::atomic<bool>* cancelFlag) {
    // Check cancellation at the start of every recursive call
    if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
        return false;

    HG_PROFILE_SCOPE("Grid::solve");
    static std::random_device rd;
    static std::mt19937       gen(rd());

    // Snapshot of cell values BEFORE we touch anything — used for perfect backtrack
    std::vector<char> cell_snapshot;
    cell_snapshot.reserve(current_word_to_fill->cells.size());
    for (Cell* cell : current_word_to_fill->cells)
        cell_snapshot.push_back(cell->value);

    // Read the actual pattern (fixed cells already have their letter)
    const std::string original_pattern = current_word_to_fill->getWord();

    if (current_word_to_fill->possible_words.empty()) {
        current_word_to_fill->possible_words = dict.getWordsByPattern(Pattern(original_pattern));
        std::shuffle(current_word_to_fill->possible_words.begin(), current_word_to_fill->possible_words.end(), gen);
    } else {
        Pattern::FilterWordsByPattern(current_word_to_fill->possible_words, Pattern(original_pattern));
    }

    // Interest indexes: cells that cross with another word
    std::vector<int> interest_indexes;
    for (size_t i = 0; i < current_word_to_fill->length; i++) {
        Cell* cell = current_word_to_fill->cells[i];
        if (cell->horizontal_word != nullptr && cell->vertical_word != nullptr)
            interest_indexes.push_back(static_cast<int>(i));
    }

    _filled.push_back(current_word_to_fill);

    const auto [r, c] = current_word_to_fill->getPosition();
    Logger::debug("Filling word at {},{} pattern: {}", r, c, original_pattern);

    std::set<std::string> tried_patterns;
    int                   avoided = 0;

    current_word_to_fill->set();
    std::vector<GridWord*> crossing_words = getCrossingWords(current_word_to_fill);

    // Helper: restore all cells to the snapshot (respects fixed cells)
    auto restore_cells = [&]() {
        for (size_t i = 0; i < current_word_to_fill->cells.size(); ++i)
            if (!current_word_to_fill->cells[i]->fixed)
                current_word_to_fill->cells[i]->value = cell_snapshot[i];
        current_word_to_fill->getWord(); // re-sync _str
    };

    for (const auto& word : current_word_to_fill->possible_words) {
        // Check cancellation on every iteration — not just on recursive entry
        if (cancelFlag && cancelFlag->load(std::memory_order_relaxed)) {
            restore_cells();
            return false;
        }

        // Must agree with every fixed cell in this GridWord
        bool conflicts_fixed = false;
        for (size_t i = 0; i < current_word_to_fill->cells.size(); ++i) {
            if (current_word_to_fill->cells[i]->fixed && current_word_to_fill->cells[i]->value != word->str[i]) {
                conflicts_fixed = true;
                break;
            }
        }
        if (conflicts_fixed) {
            ++avoided;
            continue;
        }

        current_word_to_fill->setWord(word->str);

        // Deduplicate by crossing-cell positions
        std::string pattern_for_interest_cells(current_word_to_fill->length, '_');
        for (int i : interest_indexes)
            pattern_for_interest_cells[i] = current_word_to_fill->cells[i]->value;

        if (tried_patterns.count(pattern_for_interest_cells) > 0) {
            ++avoided;
            restore_cells();
            continue;
        }
        tried_patterns.insert(pattern_for_interest_cells);

        // Forward-checking — skip fully-fixed crossing words: they're already satisfied
        bool can_advance = true;
        for (GridWord* crossing_word : crossing_words) {
            if (crossing_word->isFullyFixed())
                continue;
            if (dict.getWordsByPattern(crossing_word->getWord()).empty()) {
                can_advance = false;
                break;
            }
        }
        if (!can_advance) {
            ++avoided;
            restore_cells();
            continue;
        }

        Logger::debug("Trying word: {} at {},{}", word->str, r, c);

        // Pick next word and recurse. Fully-fixed words in _to_fill are moved
        // to _filled inline, but we must remember how many we pushed so we can
        // pop them back on backtrack.
        std::vector<GridWord*> inlined_fixed; // fully-fixed words consumed here

        bool solved = false;
        while (true) {
            GridWord* candidate = getNextGridWordToFill(_to_fill, _filled, dict);
            if (candidate == nullptr) {
                solved = true;
                break;
            } // grid complete

            _to_fill.erase(std::remove(_to_fill.begin(), _to_fill.end(), candidate), _to_fill.end());

            if (candidate->isFullyFixed()) {
                candidate->set();
                _filled.push_back(candidate);
                inlined_fixed.push_back(candidate);
                continue;
            }

            if (solve(candidate, _to_fill, _filled, dict, cancelFlag)) {
                solved = true;
                break;
            }

            _to_fill.push_back(candidate);
            break;
        }

        if (solved)
            return true;

        // Backtrack: remove any fully-fixed words we inlined
        for (GridWord* fw : inlined_fixed) {
            fw->unset();
            _filled.erase(std::remove(_filled.begin(), _filled.end(), fw), _filled.end());
            _to_fill.push_back(fw);
        }

        restore_cells();
    }

    if (avoided > 0)
        Logger::debug("Backtracking from {},{} pattern: {}. Avoided {}.", r, c, original_pattern, avoided);

    _filled.pop_back();
    restore_cells();
    current_word_to_fill->possible_words.clear();
    current_word_to_fill->unset();

    return false;
}

std::vector<GridWord*> Grid::getCrossingWords(const GridWord* word) const {
    std::vector<GridWord*> crossing_words;
    for (Cell* cell : word->cells) {
        if (word->direction == GridWordDirection::ACROSS && cell->vertical_word != nullptr &&
            !cell->vertical_word->isSet()) {
            crossing_words.push_back(cell->vertical_word);
        } else if (word->direction == GridWordDirection::DOWN && cell->horizontal_word != nullptr &&
                   !cell->horizontal_word->isSet()) {
            crossing_words.push_back(cell->horizontal_word);
        }
    }
    return crossing_words;
}

// GridWord* Grid::getNextGridWordToFill(const std::vector<GridWord*>& _to_fill, const std::vector<GridWord*>& _filled)
// {
//     HG_PROFILE_SCOPE("Grid::getNextGridWordToFill");
//     // This function should return the next grid word to fill based on some heuristic, such as the one with the
//     fewest possible words that can fit in it. This is a common heuristic used in backtracking algorithms to reduce
//     the search space and find solutions faster.

//     if (_to_fill.empty())
//     {
//         return nullptr;
//     }

//     if (_filled.empty())
//     {
//         return _to_fill[0]; // If no words have been filled yet, return the first word in the _to_fill vector
//     }

//     // Placeholder implementation: return a word that crosses with the most recently filled word. If not, return a
//     word that crosses the last before filled word, and so on. If no word crosses with any of the filled words, return
//     the first word in the _to_fill vector.
//     // TODO: implement the heuristic to select the next grid word to fill
//     // THIS IS WRONG AND NEEDS TO BE FIXED, BECAUSE IT CAN LEAD TO INFINITE LOOPS IF THE GRID WORDS ARE NOT PROPERLY
//     CONNECTED. WE NEED TO IMPLEMENT A BETTER HEURISTIC TO SELECT THE NEXT GRID WORD TO FILL, SUCH AS THE ONE WITH THE
//     FEWEST POSSIBLE WORDS THAT CAN FIT IN IT. for ( GridWord* _filled_word : _filled )
//     {
//         for (GridWord* _candidate : _to_fill)
//         {
//             if (crossing(_candidate, _filled_word))
//             {
//                 return _candidate;
//             }
//         }
//     }

//     /**
//      * In the case where remaining grid words are totally separated from the filled grid words, we can just return
//      the first word in the _to_fill vector, since there is no heuristic that can help us select a better word to
//      fill.
//      */
//     return _to_fill[0];
// }

GridWord* Grid::getNextGridWordToFill(const std::vector<GridWord*>& _to_fill, const std::vector<GridWord*>& _filled,
                                      const Dict& dict) {
    HG_PROFILE_SCOPE("Grid::getNextGridWordToFill");

    if (_to_fill.empty())
        return nullptr;
    if (_filled.empty())
        return _to_fill[0];

    GridWord* best       = nullptr;
    int       best_count = INT_MAX;

    for (GridWord* candidate : _to_fill) {
        // Only consider words that cross with at least one filled word
        bool crosses = false;
        for (GridWord* filled : _filled) {
            if (crossing(candidate, filled)) {
                crosses = true;
                break;
            }
        }
        if (!crosses)
            continue;

        // MRV: count how many words can still fit in this candidate
        // Skip fully-fixed candidates — they're already satisfied, not a constraint
        if (candidate->isFullyFixed())
            continue;

        // The fewer options, the higher priority — fail early
        int count = static_cast<int>(dict.getWordsByPattern(candidate->getWord()).size());

        if (count == 0)
            return candidate; // Already dead end — pick it immediately to fail fast

        if (count < best_count) {
            best_count = count;
            best       = candidate;
        }
    }

    // If no crossing word found, return first in _to_fill
    return best ? best : _to_fill[0];
}

bool Grid::crossing(const GridWord* word1, const GridWord* word2) {
    HG_PROFILE_SCOPE("Grid::crossing");
    // This function should check if two grid words cross each other, which means that they share at least one cell.
    // This is important for the backtracking algorithm to ensure that when we fill a grid word, we also update the
    // crossing grid words accordingly.

    // check direction is opposite
    if (word1->direction == word2->direction) {
        return false; // Words in the same direction cannot cross each other
    }

    // check cells:
    for (Cell* cell1 : word1->cells) {
        for (Cell* cell2 : word2->cells) {
            if (cell1 == cell2) {
                return true; // The two grid words cross each other
            }
        }
    }

    return false; // The two grid words do not cross each other
}