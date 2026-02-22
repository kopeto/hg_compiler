#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QString>
#include <memory>

#include "crossword.h"
#include "dict.h"

// ──────────────────────────────────────────────────────────────
// Worker that runs Grid::solve() on a separate thread and emits
// a snapshot of the current cell values periodically so the UI
// can be updated without coupling the solver to Qt directly.
// ──────────────────────────────────────────────────────────────
class SolverWorker : public QObject {
    Q_OBJECT
public:
    explicit SolverWorker(Grid* grid, const Dict* dict, QObject* parent = nullptr);

signals:
    // Emitted when the solver finishes (success=true means a solution was found)
    void finished(bool success);
    // Emitted periodically with a snapshot of all cell values (row-major, char per cell)
    void gridSnapshot(QVector<QVector<char>> snapshot);

public slots:
    void run();

private:
    Grid*       _grid;
    const Dict* _dict;
};

// ──────────────────────────────────────────────────────────────
// Custom widget that displays a single crossword cell.
// Black cells are painted solid black; fillable cells show
// an editable letter centred in a white square.
// ──────────────────────────────────────────────────────────────
class CellWidget : public QWidget {
    Q_OBJECT
public:
    explicit CellWidget(bool isBlack, QWidget* parent = nullptr);

    void setLetter(char c);
    char letter() const { return _letter; }
    bool isBlack() const { return _isBlack; }
    void setBlack(bool black);

signals:
    void clicked(CellWidget* cell);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

public:
    static constexpr int CELL_SIZE = 36;

private:
    bool _isBlack  = false;
    char _letter   = '_';
};

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

    void rebuildLayout();
};

// ──────────────────────────────────────────────────────────────
// Main application window
// ──────────────────────────────────────────────────────────────
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Menu → File
    void onNewGrid();
    void onOpenGrid();
    void onSaveGrid();

    // Menu → Solver
    void onSolve();
    void onStopSolver();

    // Solver callbacks
    void onSolverFinished(bool success);

    // Periodic UI refresh while solving
    void onRefreshTimer();

private:
    // ── UI ──
    void setupMenuBar();
    void setupCentralWidget();

    GridWidget* _gridWidget   = nullptr;
    QLabel*     _statusLabel  = nullptr;   // right panel placeholder for now

    // ── Domain ──
    std::unique_ptr<Crossword> _crossword;
    std::unique_ptr<Dict>      _dict;
    std::string                _currentGridPath;  // path del grid activo

    void loadDefaultGrid();

    // ── Solver thread ──
    QThread*      _solverThread = nullptr;
    SolverWorker* _solverWorker = nullptr;
    QTimer*       _refreshTimer;          // fires at 4 Hz to push snapshots to UI

    QMutex                  _snapshotMutex;
    QVector<QVector<char>>  _latestSnapshot;
    bool                    _hasNewSnapshot = false;

    bool _solving = false;
    void stopSolver();
};