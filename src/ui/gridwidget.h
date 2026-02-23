#pragma once

#include <QWidget>
#include <QVector>

#include "cellwidget.h"
#include "grid.h"

// ──────────────────────────────────────────────────────────────
// Grid widget: a QWidget that lays out CellWidgets in a grid.
// ──────────────────────────────────────────────────────────────
class GridWidget : public QWidget {
    Q_OBJECT
public:
    explicit GridWidget(QWidget* parent = nullptr);

    // Populate from a loaded Grid object
    void loadFromGrid(const Grid& grid);

    // Apply a snapshot received from the solver worker
    void applySnapshot(const QVector<QVector<char>>& snapshot);

    // Read back current state as char grid ('#' / letter / '_')
    QVector<QVector<char>> toCharGrid() const;

    int rows() const { return _rows; }
    int cols() const { return _cols; }

private:
    int _rows = 0;
    int _cols = 0;
    QVector<QVector<CellWidget*>> _cells;
};
