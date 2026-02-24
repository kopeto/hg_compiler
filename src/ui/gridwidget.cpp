#include "gridwidget.h"

#include <QGridLayout>

GridWidget::GridWidget(QWidget* parent) : QWidget(parent) {}

// ── helpers ───────────────────────────────────────────────────

bool GridWidget::hasWordInDir(int r, int c, GridWordDirection dir) const {
    if (r < 0 || r >= _rows || c < 0 || c >= _cols)
        return false;
    if (_cells[r][c]->isBlack())
        return false;

    if (dir == GridWordDirection::ACROSS) {
        // Part of an across word if there is at least one white neighbour to left or right
        bool leftWhite  = (c > 0 && !_cells[r][c - 1]->isBlack());
        bool rightWhite = (c < _cols - 1 && !_cells[r][c + 1]->isBlack());
        return leftWhite || rightWhite;
    } else {
        bool upWhite   = (r > 0 && !_cells[r - 1][c]->isBlack());
        bool downWhite = (r < _rows - 1 && !_cells[r + 1][c]->isBlack());
        return upWhite || downWhite;
    }
}

void GridWidget::updateSelectionHighlight() {
    // Clear all highlights first
    for (int r = 0; r < _rows; ++r)
        for (int c = 0; c < _cols; ++c)
            _cells[r][c]->setHighlight(0);

    if (_selRow < 0 || _selCol < 0)
        return;

    // Highlight the whole word
    if (_selDir == GridWordDirection::ACROSS) {
        // Walk left to find start
        int start = _selCol;
        while (start > 0 && !_cells[_selRow][start - 1]->isBlack())
            --start;
        int end = _selCol;
        while (end < _cols - 1 && !_cells[_selRow][end + 1]->isBlack())
            ++end;
        for (int c = start; c <= end; ++c)
            _cells[_selRow][c]->setHighlight(1);
    } else {
        int start = _selRow;
        while (start > 0 && !_cells[start - 1][_selCol]->isBlack())
            --start;
        int end = _selRow;
        while (end < _rows - 1 && !_cells[end + 1][_selCol]->isBlack())
            ++end;
        for (int r = start; r <= end; ++r)
            _cells[r][_selCol]->setHighlight(1);
    }

    // Active cell on top
    _cells[_selRow][_selCol]->setHighlight(2);
}

void GridWidget::selectCell(int r, int c, GridWordDirection dir) {
    if (r < 0 || r >= _rows || c < 0 || c >= _cols)
        return;
    if (_cells[r][c]->isBlack())
        return;

    _selRow = r;
    _selCol = c;
    _selDir = dir;
    updateSelectionHighlight();
    _cells[r][c]->setFocus();
    emit selectionChanged(r, c, dir);
}

void GridWidget::onKeyNavigate(int fromR, int fromC, int key) {
    int dr = 0, dc = 0;

    switch (key) {
    // Arrow keys: move one step in that direction, reorient accordingly
    case Qt::Key_Right:
        dc = +1;
        break;
    case Qt::Key_Left:
        dc = -1;
        break;
    case Qt::Key_Down:
        dr = +1;
        break;
    case Qt::Key_Up:
        dr = -1;
        break;
    // Tab = advance along the currently selected word direction
    case Qt::Key_Tab:
        if (_selDir == GridWordDirection::ACROSS)
            dc = +1;
        else
            dr = +1;
        break;
    // Backspace = step back along the currently selected word direction
    case Qt::Key_Backspace:
        if (_selDir == GridWordDirection::ACROSS)
            dc = -1;
        else
            dr = -1;
        break;
    default:
        return;
    }

    int nr = fromR + dr;
    int nc = fromC + dc;

    if (nr < 0 || nr >= _rows || nc < 0 || nc >= _cols)
        return;
    if (_cells[nr][nc]->isBlack())
        return; // black cell = wall

    // Determine new direction:
    // Arrow keys reorient; Tab/Backspace keep current direction
    GridWordDirection newDir = _selDir;
    if (key == Qt::Key_Left || key == Qt::Key_Right)
        newDir = GridWordDirection::ACROSS;
    else if (key == Qt::Key_Up || key == Qt::Key_Down)
        newDir = GridWordDirection::DOWN;

    // If the target cell doesn't have a word in newDir, fall back to the other
    if (!hasWordInDir(nr, nc, newDir)) {
        GridWordDirection other =
            (newDir == GridWordDirection::ACROSS) ? GridWordDirection::DOWN : GridWordDirection::ACROSS;
        if (hasWordInDir(nr, nc, other))
            newDir = other;
    }

    selectCell(nr, nc, newDir);
}

void GridWidget::connectCell(CellWidget* cw, int r, int c) {
    // Left-click: select cell / toggle direction
    connect(cw, &CellWidget::clicked, this, [this, r, c](CellWidget* cell) {
        if (cell->isBlack())
            return;
        emit interactionRequested(); // pause solver synchronously before touching anything

        if (_selRow == r && _selCol == c) {
            GridWordDirection other =
                (_selDir == GridWordDirection::ACROSS) ? GridWordDirection::DOWN : GridWordDirection::ACROSS;
            if (hasWordInDir(r, c, other))
                selectCell(r, c, other);
        } else {
            GridWordDirection dir =
                hasWordInDir(r, c, GridWordDirection::ACROSS) ? GridWordDirection::ACROSS : GridWordDirection::DOWN;
            selectCell(r, c, dir);
        }
    });

    // Keyboard navigation (arrows, backspace-back, advance-after-letter)
    connect(cw, &CellWidget::keyNavigate, this, [this, r, c](CellWidget*, int key) {
        // Only navigation keys that write data need to pause the solver
        if (key != Qt::Key_Left && key != Qt::Key_Right && key != Qt::Key_Up && key != Qt::Key_Down)
            emit interactionRequested();
        onKeyNavigate(r, c, key);
    });

    // Right-click in edit mode:
    // Right-click in edit mode:
    //   - cell has a letter (fixed or solver-placed) → clear the letter
    //   - cell is empty (blank white cell)           → toggle black
    connect(cw, &CellWidget::rightClicked, this, [this, r, c](CellWidget* cell) {
        if (!_editMode)
            return;
        emit interactionRequested(); // pause solver before any modification
        if (!cell->isBlack() && cell->letter() != '_') {
            // Has a letter — just erase it (unfixing if needed)
            cell->setFixed(false);
            cell->setLetter('_');
            emit cellFixed(r, c, '_', false);
        } else if (!cell->isBlack()) {
            // Empty white cell → toggle black
            cell->setBlack(true);
            QMetaObject::invokeMethod(this, [this]() { emit gridModified(); }, Qt::QueuedConnection);
        } else {
            // Black cell → toggle back to white
            cell->setBlack(false);
            QMetaObject::invokeMethod(this, [this]() { emit gridModified(); }, Qt::QueuedConnection);
        }
    });

    // Key press: letter written/erased → emit cellFixed so domain Grid is updated
    connect(cw, &CellWidget::fixToggled, this, [this, r, c](CellWidget* cell) {
        emit interactionRequested(); // pause solver before writing to the cell
        emit cellFixed(r, c, cell->letter(), cell->isFixed());
    });
}

void GridWidget::buildLayout() {
    // Delete old layout
    if (layout()) {
        QLayoutItem* item;
        while ((item = layout()->takeAt(0)) != nullptr)
            delete item;
        delete layout();
    }

    auto* gl = new QGridLayout(this);
    gl->setSpacing(0);
    gl->setContentsMargins(0, 0, 0, 0);

    for (int r = 0; r < _rows; ++r)
        for (int c = 0; c < _cols; ++c)
            gl->addWidget(_cells[r][c], r, c);

    setFixedSize(_cols * CellWidget::CELL_SIZE, _rows * CellWidget::CELL_SIZE);
}

// ── public API ────────────────────────────────────────────────

void GridWidget::loadFromGrid(const Grid& grid) {
    for (auto& row : _cells)
        for (auto* cell : row)
            delete cell;
    _cells.clear();
    _selRow = -1;
    _selCol = -1;

    _rows = grid.getRows();
    _cols = grid.getCols();
    _cells.resize(_rows, QVector<CellWidget*>(_cols, nullptr));

    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            bool  black = (grid.getValue(r, c) == '#');
            auto* cw    = new CellWidget(black, this);
            if (!black) {
                char v = static_cast<char>(grid.getValue(r, c));
                if (v != '_' && v != '.') {
                    cw->setLetter(v);
                    if (grid.isFixed(r, c))
                        cw->setFixed(true);
                }
            }
            connectCell(cw, r, c);
            _cells[r][c] = cw;
        }
    }

    buildLayout();
}

void GridWidget::loadBlank(int rows, int cols) {
    for (auto& row : _cells)
        for (auto* cell : row)
            delete cell;
    _cells.clear();
    _selRow = -1;
    _selCol = -1;

    _rows = rows;
    _cols = cols;
    _cells.resize(_rows, QVector<CellWidget*>(_cols, nullptr));

    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            auto* cw = new CellWidget(/*isBlack=*/false, this);
            connectCell(cw, r, c);
            _cells[r][c] = cw;
        }
    }

    buildLayout();
}

void GridWidget::setEditMode(bool on) {
    _editMode = on;
}

void GridWidget::setCellBlack(int row, int col, bool black) {
    if (row >= 0 && row < _rows && col >= 0 && col < _cols)
        _cells[row][col]->setBlack(black);
}

void GridWidget::setCellFixed(int row, int col, char letter) {
    if (row >= 0 && row < _rows && col >= 0 && col < _cols && !_cells[row][col]->isBlack()) {
        _cells[row][col]->setLetter(letter);
        _cells[row][col]->setFixed(true);
    }
}

void GridWidget::applySnapshot(const QVector<QVector<char>>& snapshot) {
    for (int r = 0; r < _rows && r < snapshot.size(); ++r)
        for (int c = 0; c < _cols && c < snapshot[r].size(); ++c)
            if (!_cells[r][c]->isBlack() && !_cells[r][c]->isFixed())
                _cells[r][c]->setLetter(snapshot[r][c]);
}

QVector<GridWidget::FixedCell> GridWidget::getFixedCells() const {
    QVector<FixedCell> result;
    for (int r = 0; r < _rows; ++r)
        for (int c = 0; c < _cols; ++c)
            if (!_cells[r][c]->isBlack() && _cells[r][c]->isFixed())
                result.push_back({r, c, _cells[r][c]->letter()});
    return result;
}

QVector<QVector<char>> GridWidget::toCharGrid() const {
    QVector<QVector<char>> result(_rows, QVector<char>(_cols, '.'));
    for (int r = 0; r < _rows; ++r)
        for (int c = 0; c < _cols; ++c)
            result[r][c] = _cells[r][c]->isBlack() ? '#' : _cells[r][c]->letter();
    return result;
}
