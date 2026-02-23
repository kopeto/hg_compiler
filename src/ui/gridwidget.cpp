#include "gridwidget.h"

#include <QGridLayout>

GridWidget::GridWidget(QWidget* parent) : QWidget(parent) {}

// ── helpers ───────────────────────────────────────────────────

void GridWidget::connectCell(CellWidget* cw, int r, int c)
{
    // Right-click: toggle black/white only in edit mode
    connect(cw, &CellWidget::rightClicked, this, [this](CellWidget* cell) {
        if (!_editMode) return;
        cell->setBlack(!cell->isBlack());
        emit gridModified();
    });
    (void)r; (void)c; // reserved for future per-cell signals
}

void GridWidget::buildLayout()
{
    // Delete old layout
    if (layout()) {
        QLayoutItem* item;
        while ((item = layout()->takeAt(0)) != nullptr) delete item;
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

void GridWidget::loadFromGrid(const Grid& grid)
{
    for (auto& row : _cells)
        for (auto* cell : row)
            delete cell;
    _cells.clear();

    _rows = grid.getRows();
    _cols = grid.getCols();
    _cells.resize(_rows, QVector<CellWidget*>(_cols, nullptr));

    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            bool black = (grid.getValue(r, c) == '#');
            auto* cw = new CellWidget(black, this);
            if (!black) {
                char v = static_cast<char>(grid.getValue(r, c));
                if (v != '_' && v != '.') cw->setLetter(v);
            }
            connectCell(cw, r, c);
            _cells[r][c] = cw;
        }
    }

    buildLayout();
}

void GridWidget::loadBlank(int rows, int cols)
{
    for (auto& row : _cells)
        for (auto* cell : row)
            delete cell;
    _cells.clear();

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

void GridWidget::setEditMode(bool on)
{
    _editMode = on;
}

void GridWidget::setCellBlack(int row, int col, bool black)
{
    if (row >= 0 && row < _rows && col >= 0 && col < _cols)
        _cells[row][col]->setBlack(black);
}

void GridWidget::applySnapshot(const QVector<QVector<char>>& snapshot)
{
    for (int r = 0; r < _rows && r < snapshot.size(); ++r)
        for (int c = 0; c < _cols && c < snapshot[r].size(); ++c)
            if (!_cells[r][c]->isBlack())
                _cells[r][c]->setLetter(snapshot[r][c]);
}

QVector<QVector<char>> GridWidget::toCharGrid() const
{
    QVector<QVector<char>> result(_rows, QVector<char>(_cols, '.'));
    for (int r = 0; r < _rows; ++r)
        for (int c = 0; c < _cols; ++c)
            result[r][c] = _cells[r][c]->isBlack() ? '#' : _cells[r][c]->letter();
    return result;
}
