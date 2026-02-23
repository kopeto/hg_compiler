#include "gridwidget.h"

#include <QGridLayout>

GridWidget::GridWidget(QWidget* parent) : QWidget(parent) {}

void GridWidget::loadFromGrid(const Grid& grid)
{
    // Clear existing cells
    for (auto& row : _cells)
        for (auto* cell : row)
            delete cell;
    _cells.clear();

    // Delete old layout if any
    if (layout()) {
        QLayoutItem* item;
        while ((item = layout()->takeAt(0)) != nullptr) delete item;
        delete layout();
    }

    _rows = grid.getRows();
    _cols = grid.getCols();
    _cells.resize(_rows, QVector<CellWidget*>(_cols, nullptr));

    auto* gl = new QGridLayout(this);
    gl->setSpacing(0);
    gl->setContentsMargins(0, 0, 0, 0);

    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            bool black = (grid.getValue(r, c) == '#');
            auto* cw = new CellWidget(black, this);
            if (!black) {
                char v = static_cast<char>(grid.getValue(r, c));
                if (v != '_' && v != '.') cw->setLetter(v);
            }
            _cells[r][c] = cw;
            gl->addWidget(cw, r, c);
        }
    }

    setFixedSize(_cols * CellWidget::CELL_SIZE, _rows * CellWidget::CELL_SIZE);
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
