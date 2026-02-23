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
#include <QListWidget>
#include <cctype>

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
    connect(_gridWidget, &GridWidget::selectionChanged,    this, &MainWindow::onSelectionChanged);
    // interactionRequested: activate cancel flag immediately (non-blocking).
    // The solver thread will finish on its own at the next recursive check.
    // No wait() here — that would deadlock the UI thread.
    connect(_gridWidget, &GridWidget::interactionRequested, this, &MainWindow::pauseSolver,
            Qt::DirectConnection);

    _gridArea = new QWidget(this);
    _gridArea->setStyleSheet("background: #e8e8e8;");
    auto* gridAreaLayout = new QVBoxLayout(_gridArea);
    gridAreaLayout->setContentsMargins(8, 8, 8, 8);
    gridAreaLayout->setSpacing(6);

    // Grid: centred horizontally, pushes to top, no vertical stretch
    gridAreaLayout->addStretch(1);
    gridAreaLayout->addWidget(_gridWidget, 0, Qt::AlignHCenter);
    gridAreaLayout->addStretch(1);

    // ── Button bar — fixed at the bottom, never overlaps the grid ──
    auto* btnBar    = new QWidget(_gridArea);
    auto* btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(6);

    auto makeBtn = [&](const QString& text, const QString& tooltip) -> QPushButton* {
        auto* btn = new QPushButton(text, btnBar);
        btn->setToolTip(tooltip);
        btn->setStyleSheet(
            "QPushButton {"
            "  background: #dcdcdc;"
            "  color: #222;"
            "  border: 1px solid #aaa;"
            "  border-radius: 4px;"
            "  padding: 4px 12px;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover  { background: #c8c8c8; border-color: #888; }"
            "QPushButton:pressed{ background: #b0b0b0; }"
            "QPushButton:disabled { color: #999; background: #ebebeb; }");
        return btn;
    };

    _resumeButton = makeBtn(tr("▶  Resume"), tr("Resume solver (F6)"));
    _resumeButton->setVisible(false);
    connect(_resumeButton, &QPushButton::clicked, this, &MainWindow::onResumeSolver);
    btnLayout->addWidget(_resumeButton);

    _clearButton = makeBtn(tr("Clear"), tr("Remove all letters, keeping only black cells"));
    connect(_clearButton, &QPushButton::clicked, this, &MainWindow::onClearGrid);
    btnLayout->addWidget(_clearButton);

    btnLayout->addStretch();

    gridAreaLayout->addWidget(btnBar, 0);   // fixed height, always below the grid

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

    // Word list: shows candidates for the currently selected word
    _wordListLabel = new QLabel(tr("Candidates:"), rightPanel);
    _wordListLabel->setStyleSheet("font-size: 11px; font-weight: bold; margin-top: 6px;");
    rightLayout->addWidget(_wordListLabel);

    _wordList = new QListWidget(rightPanel);
    _wordList->setAlternatingRowColors(true);
    _wordList->setStyleSheet("font-family: monospace; font-size: 12px;");
    connect(_wordList, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onWordListDoubleClicked);
    rightLayout->addWidget(_wordList, /*stretch=*/1);

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

    // Refresh the candidate list — fixed letters change the pattern
    updateWordList(_gridWidget->selectedRow(), _gridWidget->selectedCol(),
                   _gridWidget->selectedDir());
}

void MainWindow::onSelectionChanged(int row, int col, GridWordDirection dir)
{
    updateWordList(row, col, dir);
}

void MainWindow::updateWordList(int row, int col, GridWordDirection dir)
{
    if (!_wordList) return;
    _wordList->clear();
    if (!_crossword || !_dict || row < 0) {
        _wordListLabel->setText(tr("Candidates:"));
        return;
    }

    const Grid& g = _crossword->getGrid();
    GridWord* gw  = g.getGridWordAt(row, col, dir);
    if (!gw) {
        _wordListLabel->setText(tr("Candidates:"));
        return;
    }

    // Build the pattern from the fixed state of each cell:
    //   fixed cell  → its letter (hard constraint)
    //   free cell   → '_' (wildcard)
    std::string patStr;
    patStr.reserve(gw->length);
    for (const Cell* cell : gw->cells)
        patStr += (cell->fixed ? static_cast<char>(std::toupper((unsigned char)cell->value)) : '_');

    Pattern pat(patStr);
    std::vector<const Word*> candidates = _dict->getWordsByPattern(pat);

    _wordListLabel->setText(tr("Candidates (%1):").arg(candidates.size()));

    for (const Word* w : candidates) {
        auto* item = new QListWidgetItem(QString::fromStdString(w->str));
        _wordList->addItem(item);
    }
}

void MainWindow::onWordListDoubleClicked(QListWidgetItem* item)
{
    if (!item || !_crossword) return;

    int row = _gridWidget->selectedRow();
    int col = _gridWidget->selectedCol();
    GridWordDirection dir = _gridWidget->selectedDir();
    if (row < 0) return;

    Grid& g       = _crossword->getGrid();
    GridWord* gw  = g.getGridWordAt(row, col, dir);
    if (!gw) return;

    std::string word = item->text().toStdString();
    if (word.size() != gw->length) return;

    // Write each letter into the grid model and the UI
    auto [startR, startC] = gw->getPosition();
    for (int i = 0; i < (int)word.size(); ++i) {
        int r = (dir == GridWordDirection::ACROSS) ? startR : startR + i;
        int c = (dir == GridWordDirection::ACROSS) ? startC + i : startC;
        char ch = static_cast<char>(std::toupper((unsigned char)word[i]));
        g.fixCell(r, c, ch);
        _gridWidget->setCellFixed(r, c, ch);
    }

    // Refresh candidate list to reflect the new (fully fixed) pattern
    updateWordList(row, col, dir);
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

void MainWindow::onClearGrid()
{
    if (!_crossword) return;
    forceStopSolver();

    Grid& g = _crossword->getGrid();

    // Unfix every cell and reset values to '_'
    for (int r = 0; r < g.getRows(); ++r)
        for (int c = 0; c < g.getCols(); ++c)
            g.unfixCell(r, c);
    g.reset();  // clears all non-fixed (now all) fillable cells to '_'

    _gridWidget->loadFromGrid(g);
    // Restore edit mode (loadFromGrid resets the widget state)
    _actEditMode->setChecked(true);
    _wordList->clear();
    _wordListLabel->setText(tr("Candidates:"));
    statusBar()->showMessage(tr("Grid cleared."));
}

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
                // Only restore the fixed cell if it still belongs to a word
                // after the grid rebuild. A cell that became isolated (no word
                // of length ≥ 2) or black must be silently dropped.
                if (!g.cellHasWord(fc.row, fc.col)) continue;
                _gridWidget->setCellFixed(fc.row, fc.col, fc.letter);
                g.fixCell(fc.row, fc.col, fc.letter);
            }

            statusBar()->showMessage(
                tr("Grid updated — %1 across, %2 down")
                    .arg(g.getAcrossWords().size())
                    .arg(g.getDownWords().size()),
                2000);

            // Refresh candidate list — word structure may have changed
            updateWordList(_gridWidget->selectedRow(), _gridWidget->selectedCol(),
                           _gridWidget->selectedDir());
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

    // Always reset non-fixed cells before a fresh solve so stale solver
    // letters from a previous run don't corrupt the pattern matching.
    grid->reset();
    _gridWidget->loadFromGrid(*grid);

    statusBar()->showMessage(tr("Solving…"));
    _statusLabel->setText(tr("⏳ Solving…"));

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
    if (!_crossword) return;

    _paused  = false;
    _solving = true;
    updateSolverActions();

    Grid* grid = &_crossword->getGrid();
    // Do NOT reset — resume from the exact state the solver left off.
    statusBar()->showMessage(tr("Solver resumed…"));
    _statusLabel->setText(tr("⏳ Solving…"));

    _solverThread = new QThread(this);
    _solverWorker = new SolverWorker(grid, _dict.get());
    _solverWorker->moveToThread(_solverThread);

    connect(_solverThread, &QThread::started,       _solverWorker, &SolverWorker::run);
    connect(_solverWorker, &SolverWorker::finished,  this,          &MainWindow::onSolverFinished);

    _refreshTimer->start();
    _solverThread->start();
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

