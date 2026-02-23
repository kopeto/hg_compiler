#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QStatusBar>
#include <QFrame>

#include "ui/newgriddialog.h"

// ═══════════════════════════════════════════════════════════════
//  MainWindow
// ═══════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Hitz Gurutzatuak");
    resize(900, 620);

    _dict     = std::make_unique<Dict>();
    _dictPath = HG_DEFAULT_DICTIONARY_PATH;

    _refreshTimer = new QTimer(this);
    _refreshTimer->setInterval(250); // 4 Hz
    connect(_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshTimer);

    setupMenuBar();
    setupCentralWidget();
    loadDefaultGrid();
}

MainWindow::~MainWindow()
{
    forceStopSolver();
}

// ── UI setup ─────────────────────────────────────────────────

void MainWindow::setupMenuBar()
{
    // ── File ──
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* actQuit = fileMenu->addAction(tr("&Quit"));
    actQuit->setShortcut(QKeySequence::Quit);
    connect(actQuit, &QAction::triggered, qApp, &QApplication::quit);

    // ── Grid ──
    QMenu* gridMenu = menuBar()->addMenu(tr("&Grid"));

    QAction* actNewBlank = gridMenu->addAction(tr("&New Blank Grid…"));
    actNewBlank->setShortcut(QKeySequence::New);
    connect(actNewBlank, &QAction::triggered, this, &MainWindow::onNewBlankGrid);

    QAction* actOpen = gridMenu->addAction(tr("&Open Grid…"));
    actOpen->setShortcut(QKeySequence::Open);
    connect(actOpen, &QAction::triggered, this, &MainWindow::onOpenGrid);

    QAction* actSave = gridMenu->addAction(tr("&Save Grid…"));
    actSave->setShortcut(QKeySequence::Save);
    connect(actSave, &QAction::triggered, this, &MainWindow::onSaveGrid);

    gridMenu->addSeparator();

    _actEditMode = gridMenu->addAction(tr("&Edit Mode (toggle black cells)"));
    _actEditMode->setCheckable(true);
    _actEditMode->setShortcut(Qt::Key_F2);
    connect(_actEditMode, &QAction::toggled, this, &MainWindow::onToggleEditMode);

    // ── Dictionary ──
    QMenu* dictMenu = menuBar()->addMenu(tr("&Dictionary"));

    QAction* actDefDict = dictMenu->addAction(tr("Load &Default Dictionary"));
    connect(actDefDict, &QAction::triggered, this, &MainWindow::onLoadDefaultDictionary);

    QAction* actCustDict = dictMenu->addAction(tr("Load &Custom Dictionary…"));
    connect(actCustDict, &QAction::triggered, this, &MainWindow::onLoadCustomDictionary);

    // ── Solver ──
    QMenu* solverMenu = menuBar()->addMenu(tr("&Solver"));

    _actSolve = solverMenu->addAction(tr("&Solve"));
    _actSolve->setShortcut(Qt::Key_F5);
    connect(_actSolve, &QAction::triggered, this, &MainWindow::onSolve);

    _actStop = solverMenu->addAction(tr("&Pause (Esc)"));
    _actStop->setShortcut(Qt::Key_Escape);
    connect(_actStop, &QAction::triggered, this, &MainWindow::pauseSolver);

    _actResume = solverMenu->addAction(tr("&Resume"));
    _actResume->setShortcut(Qt::Key_F6);
    _actResume->setEnabled(false);
    connect(_actResume, &QAction::triggered, this, &MainWindow::onResumeSolver);
}

void MainWindow::setupCentralWidget()
{
    auto* centralWidget = new QWidget(this);
    auto* mainLayout    = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    // ── Left: container that centres the grid ──
    _gridWidget = new GridWidget(this);
    connect(_gridWidget, &GridWidget::gridModified,        this, &MainWindow::onGridModified);
    connect(_gridWidget, &GridWidget::cellFixed,           this, &MainWindow::onCellFixed);
    // interactionRequested: activate cancel flag immediately (non-blocking).
    // The solver thread will finish on its own at the next recursive check.
    // No wait() here — that would deadlock the UI thread.
    connect(_gridWidget, &GridWidget::interactionRequested, this, &MainWindow::pauseSolver,
            Qt::DirectConnection);

    _gridArea = new QWidget(this);
    _gridArea->setStyleSheet("background: #e8e8e8;");
    auto* gridAreaLayout = new QHBoxLayout(_gridArea);
    gridAreaLayout->setContentsMargins(8, 8, 8, 8);
    gridAreaLayout->addWidget(_gridWidget, 0, Qt::AlignCenter);

    mainLayout->addWidget(_gridArea, /*stretch=*/3);

    // ── Right panel ──
    auto* rightPanel = new QFrame(this);
    rightPanel->setFrameShape(QFrame::StyledPanel);
    rightPanel->setMinimumWidth(200);

    auto* rightLayout = new QVBoxLayout(rightPanel);

    _dictLabel = new QLabel(this);
    _dictLabel->setWordWrap(true);
    _dictLabel->setStyleSheet("font-size: 11px; color: #555;");
    rightLayout->addWidget(_dictLabel);
    updateDictLabel();

    _editModeLabel = new QLabel(this);
    _editModeLabel->setStyleSheet("font-size: 11px;");
    rightLayout->addWidget(_editModeLabel);
    updateEditModeIndicator();

    _statusLabel = new QLabel(tr("Ready"), rightPanel);
    _statusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    _statusLabel->setWordWrap(true);
    rightLayout->addWidget(_statusLabel);

    // Resume button (visible only when paused mid-solve)
    _resumeButton = new QPushButton(tr("▶  Resume (F6)"), rightPanel);
    _resumeButton->setVisible(false);
    _resumeButton->setStyleSheet(
        "QPushButton { background:#2e7d32; color:white; font-weight:bold;"
        " border-radius:4px; padding:6px; }"
        "QPushButton:hover { background:#388e3c; }");
    connect(_resumeButton, &QPushButton::clicked, this, &MainWindow::onResumeSolver);
    rightLayout->addWidget(_resumeButton);

    rightLayout->addStretch();

    mainLayout->addWidget(rightPanel, /*stretch=*/1);

    setCentralWidget(centralWidget);
    statusBar()->showMessage(tr("Ready"));
}

// ── Domain helpers ────────────────────────────────────────────

void MainWindow::loadDefaultGrid()
{
    try {
        _currentGridPath = HG_DEFAULT_GRID_PATH;
        _crossword = std::make_unique<Crossword>(_currentGridPath);
        _gridWidget->loadFromGrid(_crossword->getGrid());
        adjustWindowForGrid();
        // Always start in edit mode
        _actEditMode->setChecked(true);
        statusBar()->showMessage(tr("Default grid loaded."));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Load error"), QString::fromStdString(e.what()));
    }
}

void MainWindow::onCellFixed(int row, int col, char letter, bool fixed)
{
    if (!_crossword) return;
    Grid& g = _crossword->getGrid();
    if (fixed)
        g.fixCell(row, col, letter);
    else
        g.unfixCell(row, col);
}

// ── Grid slots ────────────────────────────────────────────────

void MainWindow::onNewBlankGrid()
{
    forceStopSolver();
    NewGridDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    // Convert the dialog's char layout → vector<string> for Grid constructor
    auto charLayout = dlg.layout();
    std::vector<std::string> lines;
    lines.reserve(charLayout.size());
    for (const auto& row : charLayout) {
        std::string line;
        line.reserve(row.size());
        for (char c : row) line += (c == '#' ? '#' : '.');
        lines.push_back(line);
    }

    try {
        _crossword       = std::make_unique<Crossword>(lines);
        _currentGridPath.clear();
        _gridWidget->loadFromGrid(_crossword->getGrid());
        adjustWindowForGrid();
        _actEditMode->setChecked(true);
        statusBar()->showMessage(tr("New %1×%2 grid created. Edit mode ON.")
                                 .arg(charLayout.size())
                                 .arg(charLayout.isEmpty() ? 0 : charLayout[0].size()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Error"), QString::fromStdString(e.what()));
    }
}

void MainWindow::onToggleEditMode(bool checked)
{
    _gridWidget->setEditMode(checked);
    updateEditModeIndicator();
    statusBar()->showMessage(checked
        ? tr("Edit mode ON — right-click a cell to toggle black/white")
        : tr("Edit mode OFF"), 3000);
}

void MainWindow::updateEditModeIndicator()
{
    if (!_editModeLabel) return;
    if (_gridWidget && _gridWidget->editMode())
        _editModeLabel->setText(tr("✏️ Edit mode ON"));
    else
        _editModeLabel->setText(tr("🔒 Edit mode OFF"));
}

void MainWindow::onOpenGrid()
{
    forceStopSolver();
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open Grid"), QString(), tr("Grid files (*.grid);;All files (*)"));
    if (path.isEmpty()) return;

    try {
        _currentGridPath = path.toStdString();
        _crossword = std::make_unique<Crossword>(_currentGridPath);
        _gridWidget->loadFromGrid(_crossword->getGrid());
        adjustWindowForGrid();
        _actEditMode->setChecked(true);
        statusBar()->showMessage(tr("Grid loaded: %1").arg(path));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Load error"), QString::fromStdString(e.what()));
    }
}

void MainWindow::onSaveGrid()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save Grid"), QString(), tr("Grid files (*.grid);;All files (*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Save error"), tr("Cannot open file for writing."));
        return;
    }
    QTextStream out(&f);

    // Save the current black/white layout (letters become '.' — grid format only
    // encodes structure, not solution)
    auto grid = _gridWidget->toCharGrid();
    for (const auto& row : grid) {
        for (char c : row) out << QChar(c == '#' ? '#' : '.');
        out << '\n';
    }
    statusBar()->showMessage(tr("Grid saved: %1").arg(path));
}

// ── Dictionary slots ─────────────────────────────────────────

void MainWindow::onLoadDefaultDictionary()
{
    try {
        _dictPath = HG_DEFAULT_DICTIONARY_PATH;
        _dict     = std::make_unique<Dict>();
        statusBar()->showMessage(tr("Default dictionary loaded."), 3000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Dictionary error"),
                              tr("Cannot load dictionary:\n%1").arg(e.what()));
        _dict = nullptr;
    }
    updateDictLabel();
}

void MainWindow::onLoadCustomDictionary()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select Dictionary"), QString(),
        tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty()) return;

    try {
        _dictPath   = path;
        _dict       = std::make_unique<Dict>();
        _dict->load(path.toStdString());
        statusBar()->showMessage(tr("Dictionary loaded: %1").arg(path), 3000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Dictionary error"),
                              tr("Cannot load dictionary:\n%1").arg(e.what()));
        _dict = nullptr;
    }
    updateDictLabel();
}

void MainWindow::updateDictLabel()
{
    if (!_dictLabel) return;
    if (_dict)
        _dictLabel->setText(tr("📖 %1").arg(QFileInfo(_dictPath).fileName()));
    else
        _dictLabel->setText(tr("⚠️ No dictionary loaded"));
}


void MainWindow::onGridModified()
{
    if (!_gridWidget->editMode()) return;

    // Snapshot fixed cells BEFORE rebuilding (loadFromGrid resets everything)
    auto fixed = _gridWidget->getFixedCells();

    auto charLayout = _gridWidget->toCharGrid();
    std::vector<std::string> lines;
    lines.reserve(charLayout.size());
    for (const auto& row : charLayout) {
        std::string line;
        line.reserve(row.size());
        for (char c : row) line += (c == '#' ? '#' : '.');
        lines.push_back(line);
    }

    // Capture by value so the lambda is safe to run after this function returns
    auto rebuildCrossword = [this, lines, fixed]() {
        try {
            _crossword = std::make_unique<Crossword>(lines);
            _gridWidget->loadFromGrid(_crossword->getGrid());
            adjustWindowForGrid();
            _gridWidget->setEditMode(true);

            Grid& g = _crossword->getGrid();
            for (const auto& fc : fixed) {
                _gridWidget->setCellFixed(fc.row, fc.col, fc.letter);
                g.fixCell(fc.row, fc.col, fc.letter);
            }

            statusBar()->showMessage(
                tr("Grid updated — %1 across, %2 down")
                    .arg(g.getAcrossWords().size())
                    .arg(g.getDownWords().size()),
                2000);
        } catch (const std::exception& e) {
            statusBar()->showMessage(tr("⚠️ %1").arg(QString::fromStdString(e.what())), 3000);
        }
    };

    if (_solving) {
        // Queue the rebuild to run once the solver has stopped cleanly.
        // pauseSolver() sets the cancel flag; onSolverFinished will call _pendingAfterStop.
        _pendingAfterStop = rebuildCrossword;
        pauseSolver();
    } else {
        rebuildCrossword();
    }
}

// ── Solver slots ─────────────────────────────────────────────

void MainWindow::adjustWindowForGrid()
{
    // Minimum size = grid pixels + margins + right panel + spacing
    const int rightPanelMin = 200;
    const int spacing       = 12;
    const int margins       = 16; // 8px each side

    int gridW = _gridWidget->width();
    int gridH = _gridWidget->height();

    // Extra space for the grid container padding (8px each side)
    int areaW = gridW + 16;
    int areaH = gridH + 16;

    // Minimum window content size
    int minW = areaW + spacing + rightPanelMin + margins;
    int minH = areaH + margins;

    // Account for menu bar and status bar heights
    int extraH = menuBar()->sizeHint().height() + statusBar()->sizeHint().height();

    setMinimumSize(minW, minH + extraH);

    // Expand current size if it's smaller than the new minimum
    int newW = qMax(width(),  minW);
    int newH = qMax(height(), minH + extraH);
    if (newW != width() || newH != height())
        resize(newW, newH);
}

void MainWindow::updateSolverActions()
{
    if (_actSolve)   _actSolve->setEnabled(!_solving);
    if (_actStop)    _actStop->setEnabled(_solving);
    if (_actResume)  _actResume->setEnabled(!_solving && _paused);
    if (_resumeButton) _resumeButton->setVisible(!_solving && _paused);
}

void MainWindow::pauseSolver()
{
    if (!_solving) return;
    _paused = true;
    if (_solverWorker)
        _solverWorker->requestCancel();
    // Never wait() here — would deadlock the UI thread.
    // The solver fires onSolverFinished via QueuedConnection when done.
}

void MainWindow::onSolve()
{
    if (_solving) return;
    if (!_crossword) { QMessageBox::information(this, tr("Solver"), tr("No grid loaded.")); return; }

    Grid* grid = &_crossword->getGrid();

    if (grid->isSolved()) {
        grid->reset();
        _gridWidget->loadFromGrid(*grid);
        statusBar()->showMessage(tr("Grid reset. Solving from scratch…"));
        _statusLabel->setText(tr("Solving from scratch…"));
    } else {
        statusBar()->showMessage(tr("Solving…"));
        _statusLabel->setText(tr("⏳ Solving…"));
    }

    _solving = true;
    _paused  = false;
    updateSolverActions();

    _solverThread = new QThread(this);
    _solverWorker = new SolverWorker(grid, _dict.get());
    _solverWorker->moveToThread(_solverThread);

    connect(_solverThread, &QThread::started,       _solverWorker, &SolverWorker::run);
    connect(_solverWorker, &SolverWorker::finished,  this,          &MainWindow::onSolverFinished);

    _refreshTimer->start();
    _solverThread->start();
}

void MainWindow::onResumeSolver()
{
    if (_solving || !_paused) return;
    _paused = false;
    onSolve();
}

void MainWindow::onStopSolver()
{
    if (!_solving) return;
    _paused = false;
    _pendingAfterStop = nullptr;
    if (_solverWorker) _solverWorker->requestCancel();
    updateSolverActions();
    statusBar()->showMessage(tr("Solver stopping…"));
}

void MainWindow::onSolverFinished(bool success)
{
    _refreshTimer->stop();
    _solving = false;

    // Thread has finished run() — safe to quit/wait now (returns immediately)
    if (_solverThread) {
        _solverThread->quit();
        _solverThread->wait();
        _solverThread->deleteLater(); _solverThread = nullptr;
    }
    if (_solverWorker) {
        _solverWorker->deleteLater(); _solverWorker = nullptr;
    }

    // Run any deferred action (e.g. grid rebuild triggered during solve)
    if (_pendingAfterStop) {
        auto action = std::move(_pendingAfterStop);
        _pendingAfterStop = nullptr;
        action();
        updateSolverActions();
        return;
    }

    if (_paused) {
        // Keep the UI exactly as it was when the solver stopped — don't refresh.
        updateSolverActions();
        statusBar()->showMessage(tr("Solver paused — press F6 or Resume to continue."));
        _statusLabel->setText(tr("⏸ Paused"));
    } else {
        // Final UI snapshot only when finishing naturally (not paused)
        onRefreshTimer();

        if (success) {
            _paused = false;
            updateSolverActions();
            statusBar()->showMessage(tr("Solution found!"));
            _statusLabel->setText(tr("✅ Solution found!"));
        } else {
            _paused = false;
            updateSolverActions();
            statusBar()->showMessage(tr("No solution found."));
            _statusLabel->setText(tr("❌ No solution found."));
        }
    }
}

void MainWindow::onRefreshTimer()
{
    if (!_crossword) return;
    const Grid& g = _crossword->getGrid();
    int rows = g.getRows();
    int cols = g.getCols();

    QVector<QVector<char>> snap(rows, QVector<char>(cols));
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            snap[r][c] = static_cast<char>(g.getValue(r, c));

    _gridWidget->applySnapshot(snap);
}

void MainWindow::forceStopSolver()
{
    // Safe to block here: only called from destructor or before a new grid load,
    // never from inside a Qt signal delivery.
    _refreshTimer->stop();
    if (_solverWorker) _solverWorker->requestCancel();
    if (_solverThread) {
        _solverThread->quit();
        _solverThread->wait();
        _solverThread->deleteLater(); _solverThread = nullptr;
    }
    if (_solverWorker) {
        _solverWorker->deleteLater(); _solverWorker = nullptr;
    }
    _solving = false;
    _pendingAfterStop = nullptr;
}

