#pragma once

#include <QWidget>
#include <QVector>

#include "cellwidget.h"
#include "grid.h"

// ──────────────────────────────────────────────────────────────
// Grid widget: a QWidget that lays out CellWidgets in a grid.
//
// Edit mode (setEditMode(true)):
//   - Right-click  → toggle cell black / white
//   - Left-click   → focus cell (normal letter input)
// ──────────────────────────────────────────────────────────────
class GridWidget : public QWidget {
    Q_OBJECT
public:
    explicit GridWidget(QWidget* parent = nullptr);

    // Populate from a loaded Grid object
    void loadFromGrid(const Grid& grid);

    // Populate with a blank NxM grid (all white cells)
    void loadBlank(int rows, int cols);

    // Directly set a cell's black state (used when importing a layout)
    void setCellBlack(int row, int col, bool black);

    // Apply a snapshot received from the solver worker
    void applySnapshot(const QVector<QVector<char>>& snapshot);

    // Read back current state as char grid ('#' / letter / '_')
    QVector<QVector<char>> toCharGrid() const;

    int rows() const { return _rows; }
    int cols() const { return _cols; }

    // When edit mode is on, right-click toggles black/white cells
    void setEditMode(bool on);
    bool editMode() const { return _editMode; }

signals:
    void gridModified(); // emitted when a cell is toggled in edit mode

private:
    void buildLayout();
    void connectCell(CellWidget* cw, int r, int c);

    int _rows     = 0;
    int _cols     = 0;
    bool _editMode = false;
    QVector<QVector<CellWidget*>> _cells;
};
