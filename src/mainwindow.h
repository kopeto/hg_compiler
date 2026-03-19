#pragma once

#include "crossword.h"
#include "dict.h"
#include "eeh_db.h"
#include "hg_config.h"
#include "ui/gridwidget.h"
#include "ui/solverworker.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <functional>
#include <memory>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onNewBlankGrid();
    void onOpenGrid();
    void onSaveGrid();
    void onExportPuz();
    void onUploadPuz();
    void onImportPuz();
    void onToggleEditMode(bool checked);

    void onLoadDefaultDictionary();
    void onLoadCustomDictionary();

    void onSolve();
    void onStopSolver();
    void onResumeSolver();
    void onClearGrid();

    void onSolverFinished(bool success);
    void onRefreshTimer();
    void onGridModified();
    void onCellFixed(int row, int col, char letter, bool fixed);
    void onSelectionChanged(int row, int col, GridWordDirection dir);
    void onWordListDoubleClicked(QListWidgetItem* item);
    void onClueChanged();

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
    void updateWordList(int row, int col, GridWordDirection dir);
    // Resize the window so it is never smaller than the grid + right panel,
    // and center the grid inside its container area.
    void adjustWindowForGrid();
    // Sync metadata text fields from _crossword.
    void syncMetaToWidgets();
    // Look up word in EEH DB and print definitions to stdout (temporary).
    void lookupWord(const QString& word);

    // ── UI ──
    GridWidget* _gridWidget  = nullptr;
    QWidget*    _gridArea    = nullptr; // container that centres GridWidget
    QLabel*     _statusLabel = nullptr;

    QPushButton*  _resumeButton      = nullptr;
    QPushButton*  _clearButton       = nullptr;
    QLineEdit*    _metaTitleEdit     = nullptr;
    QLineEdit*    _metaAuthorEdit    = nullptr;
    QLineEdit*    _metaCopyrightEdit = nullptr;
    QLabel*       _wordListLabel     = nullptr;
    QListWidget*  _wordList          = nullptr;
    QTextEdit*    _clueEdit          = nullptr;
    QTextBrowser* _eehBrowser        = nullptr; // panel to show definitions/examples
    QAction*      _actEditMode       = nullptr;
    QAction*      _actSymmetry       = nullptr;
    QAction*      _actSolve          = nullptr;
    QAction*      _actStop           = nullptr;
    QAction*      _actResume         = nullptr;
    QToolBar*     _toolbar           = nullptr;

    // ── Domain ──
    std::unique_ptr<Crossword> _crossword;
    std::unique_ptr<Dict>      _dict;
    // EEH DB is accessed via a worker thread to avoid blocking the UI.
    QThread*         _eehThread = nullptr;
    class EehWorker* _eehWorker = nullptr;
    std::string      _currentGridPath;
    QString          _dictPath;
    HgConfig         _config;

    // ── Solver thread ──
    QThread*      _solverThread = nullptr;
    SolverWorker* _solverWorker = nullptr;
    QTimer*       _refreshTimer = nullptr;

    bool _solving = false;
    bool _paused  = false;

    // Action deferred until the solver thread has stopped.
    // Set by pauseSolver(); executed at the top of onSolverFinished().
    std::function<void()> _pendingAfterStop;

signals:
    // Request worker to perform a lookup (queued connection)
    void requestEehLookup(const QString& word);

private slots:
    // Called when worker finishes a lookup
    void onEehLookupDone(const QString& word, bool found, const QStringList& defs, const QStringList& examples);
};