#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QThread>
#include <functional>
#include <memory>

#include "crossword.h"
#include "dict.h"
#include "ui/gridwidget.h"
#include "ui/solverworker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onNewBlankGrid();
    void onOpenGrid();
    void onSaveGrid();
    void onToggleEditMode(bool checked);

    void onLoadDefaultDictionary();
    void onLoadCustomDictionary();

    void onSolve();
    void onStopSolver();
    void onResumeSolver();

    void onSolverFinished(bool success);
    void onRefreshTimer();
    void onGridModified();
    void onCellFixed(int row, int col, char letter, bool fixed);

    // Activated by GridWidget::interactionRequested (DirectConnection).
    // Sets cancel flag; queues a pending action to run once the solver stops.
    void pauseSolver();

private:
    void setupMenuBar();
    void setupCentralWidget();
    void loadDefaultGrid();
    // Unconditionally kill the solver thread (safe to call from destructor /
    // grid-load because those happen outside the Qt event loop delivery).
    void forceStopSolver();
    void updateDictLabel();
    void updateEditModeIndicator();
    void updateSolverActions();

    // ── UI ──
    GridWidget*  _gridWidget    = nullptr;
    QLabel*      _statusLabel   = nullptr;
    QLabel*      _dictLabel     = nullptr;
    QLabel*      _editModeLabel = nullptr;
    QPushButton* _resumeButton  = nullptr;
    QAction*     _actEditMode   = nullptr;
    QAction*     _actSolve      = nullptr;
    QAction*     _actStop       = nullptr;
    QAction*     _actResume     = nullptr;

    // ── Domain ──
    std::unique_ptr<Crossword> _crossword;
    std::unique_ptr<Dict>      _dict;
    std::string                _currentGridPath;
    QString                    _dictPath;

    // ── Solver thread ──
    QThread*      _solverThread = nullptr;
    SolverWorker* _solverWorker = nullptr;
    QTimer*       _refreshTimer = nullptr;

    bool _solving = false;
    bool _paused  = false;

    // Action deferred until the solver thread has stopped.
    // Set by pauseSolver(); executed at the top of onSolverFinished().
    std::function<void()> _pendingAfterStop;
};