#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <memory>

#include "crossword.h"
#include "dict.h"
#include "ui/gridwidget.h"
#include "ui/solverworker.h"

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

    // Menu → Dictionary
    void onLoadDefaultDictionary();
    void onLoadCustomDictionary();

    // Menu → Solver
    void onSolve();
    void onStopSolver();

    // Solver callbacks
    void onSolverFinished(bool success);

    // Periodic UI refresh while solving
    void onRefreshTimer();

private:
    void setupMenuBar();
    void setupCentralWidget();
    void loadDefaultGrid();
    void stopSolver();
    void updateDictLabel();

    // ── UI ──
    GridWidget* _gridWidget  = nullptr;
    QLabel*     _statusLabel = nullptr;
    QLabel*     _dictLabel   = nullptr;

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
};