#pragma once

#include <QWidget>
#include <QVector>

#include "cellwidget.h"
#include "grid.h"
#include "grid_word.h"

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

    // Fix a cell with a given letter (used to restore fixed state after rebuild)
    void setCellFixed(int row, int col, char letter);

    // Apply a snapshot received from the solver worker
    void applySnapshot(const QVector<QVector<char>>& snapshot);

    // Read back current state as char grid ('#' / letter / '_')
    QVector<QVector<char>> toCharGrid() const;

    int rows() const { return _rows; }
    int cols() const { return _cols; }

    // When edit mode is on, right-click toggles black/white cells
    void setEditMode(bool on);
    bool editMode() const { return _editMode; }

    // Returns {row, col, letter} for every currently fixed cell
    struct FixedCell { int row, col; char letter; };
    QVector<FixedCell> getFixedCells() const;

    // Current selection
    int  selectedRow() const { return _selRow; }
    int  selectedCol() const { return _selCol; }
    GridWordDirection selectedDir() const { return _selDir; }

signals:
    void gridModified();   // cell toggled black/white in edit mode
    void cellFixed(int row, int col, char letter, bool fixed); // letter locked/unlocked
    // Emitted whenever the selected cell or direction changes
    void selectionChanged(int row, int col, GridWordDirection dir);
    // Emitted BEFORE any action that could modify the grid or domain state.
    // MainWindow connects this to pauseSolver() so the solver thread is stopped
    // before the UI thread touches any shared data.
    void interactionRequested();

private:
    void buildLayout();
    void connectCell(CellWidget* cw, int r, int c);

    // Returns true if the cell at (r,c) belongs to a word in the given direction.
    // Deduced from neighbouring cells (no need for a Grid reference).
    bool hasWordInDir(int r, int c, GridWordDirection dir) const;

    // Repaint selection highlight across all cells
    void updateSelectionHighlight();

    // Move selection to (r,c) with given direction; clamps and skips blacks
    void selectCell(int r, int c, GridWordDirection dir);

    // Handle keyboard navigation from a cell
    void onKeyNavigate(int fromR, int fromC, int key);

    int _rows     = 0;
    int _cols     = 0;
    bool _editMode = false;
    QVector<QVector<CellWidget*>> _cells;

    // Current selection (-1 = none)
    int _selRow = -1;
    int _selCol = -1;
    GridWordDirection _selDir = GridWordDirection::ACROSS;
};
