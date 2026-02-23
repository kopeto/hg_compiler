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
#include <QScrollArea>

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
    stopSolver();
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

    QAction* actSolve = solverMenu->addAction(tr("&Solve"));
    actSolve->setShortcut(Qt::Key_F5);
    connect(actSolve, &QAction::triggered, this, &MainWindow::onSolve);

    QAction* actStop = solverMenu->addAction(tr("S&top"));
    actStop->setShortcut(Qt::Key_Escape);
    connect(actStop, &QAction::triggered, this, &MainWindow::onStopSolver);
}

void MainWindow::setupCentralWidget()
{
    auto* centralWidget = new QWidget(this);
    auto* mainLayout    = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    // ── Left: scroll area containing the grid ──
    _gridWidget = new GridWidget(this);
    connect(_gridWidget, &GridWidget::gridModified, this, &MainWindow::onGridModified);

    auto* scroll = new QScrollArea(this);
    scroll->setWidget(_gridWidget);
    scroll->setWidgetResizable(false);
    scroll->setFrameShape(QFrame::StyledPanel);
    mainLayout->addWidget(scroll, /*stretch=*/3);

    // ── Right: placeholder panel ──
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
        statusBar()->showMessage(tr("Default grid loaded."));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Load error"), QString::fromStdString(e.what()));
    }
}

// ── Grid slots ────────────────────────────────────────────────

void MainWindow::onNewBlankGrid()
{
    stopSolver();
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
    stopSolver();
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open Grid"), QString(), tr("Grid files (*.grid);;All files (*)"));
    if (path.isEmpty()) return;

    try {
        _currentGridPath = path.toStdString();
        _crossword = std::make_unique<Crossword>(_currentGridPath);
        _gridWidget->loadFromGrid(_crossword->getGrid());
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

    // Snapshot the current UI layout → vector<string> of '.' and '#'
    auto charLayout = _gridWidget->toCharGrid();
    std::vector<std::string> lines;
    lines.reserve(charLayout.size());
    for (const auto& row : charLayout) {
        std::string line;
        line.reserve(row.size());
        for (char c : row) line += (c == '#' ? '#' : '.');
        lines.push_back(line);
    }

    try {
        _crossword = std::make_unique<Crossword>(lines);
        // Reload the grid widget from the freshly built domain Grid so cell
        // pointers and GridWord metadata are perfectly in sync with the UI.
        _gridWidget->loadFromGrid(_crossword->getGrid());
        // Keep edit mode active after reload
        _gridWidget->setEditMode(true);

        const Grid& g = _crossword->getGrid();
        statusBar()->showMessage(
            tr("Grid updated — %1 across, %2 down")
                .arg(g.getAcrossWords().size())
                .arg(g.getDownWords().size()),
            2000);
    } catch (const std::exception& e) {
        // Invalid layout (e.g. all white — no words): just report, don't crash
        statusBar()->showMessage(tr("⚠️ %1").arg(QString::fromStdString(e.what())), 3000);
    }
}

// ── Solver slots ─────────────────────────────────────────────

void MainWindow::onSolve()
{
    if (_solving) return;
    if (!_crossword) { QMessageBox::information(this, tr("Solver"), tr("No grid loaded.")); return; }

    Grid* grid = &_crossword->getGrid();

    // ── Path 1: grid already solved → reset and start from scratch ──
    if (grid->isSolved()) {
        grid->reset();
        _gridWidget->loadFromGrid(*grid);   // refresh UI to empty state
        statusBar()->showMessage(tr("Grid reset. Solving from scratch…"));
        _statusLabel->setText(tr("Solving from scratch…"));
    }
    // ── Path 2: intermediate state → continue from current state ──
    else {
        statusBar()->showMessage(tr("Continuing from current state…"));
        _statusLabel->setText(tr("Continuing…"));
    }

    _solving = true;

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
    stopSolver();
    statusBar()->showMessage(tr("Solver stopped."));
    _statusLabel->setText(tr("Solver stopped."));
}

void MainWindow::onSolverFinished(bool success)
{
    _refreshTimer->stop();
    _solving = false;

    // Final snapshot
    onRefreshTimer();

    _solverThread->quit();
    _solverThread->wait();
    _solverThread->deleteLater(); _solverThread = nullptr;
    _solverWorker->deleteLater(); _solverWorker = nullptr;

    if (success) {
        statusBar()->showMessage(tr("Solution found!"));
        _statusLabel->setText(tr("✅ Solution found!"));
    } else {
        statusBar()->showMessage(tr("No solution found."));
        _statusLabel->setText(tr("❌ No solution found."));
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

void MainWindow::stopSolver()
{
    if (!_solving) return;
    _refreshTimer->stop();
    _solving = false;
    if (_solverThread) {
        _solverThread->requestInterruption();
        _solverThread->quit();
        _solverThread->wait(3000);
        _solverThread->deleteLater(); _solverThread = nullptr;
    }
    if (_solverWorker) {
        _solverWorker->deleteLater(); _solverWorker = nullptr;
    }
}
