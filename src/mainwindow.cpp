#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStatusBar>
#include <QDockWidget>
#include <QFrame>
#include <QMutexLocker>
#include <QScrollArea>

// ═══════════════════════════════════════════════════════════════
//  SolverWorker
// ═══════════════════════════════════════════════════════════════

SolverWorker::SolverWorker(Grid* grid, const Dict* dict, QObject* parent)
    : QObject(parent), _grid(grid), _dict(dict) {}

void SolverWorker::run()
{
    bool ok = _grid->solve(*_dict);
    emit finished(ok);
}

// ═══════════════════════════════════════════════════════════════
//  CellWidget
// ═══════════════════════════════════════════════════════════════

CellWidget::CellWidget(bool isBlack, QWidget* parent)
    : QWidget(parent), _isBlack(isBlack)
{
    setFixedSize(CELL_SIZE, CELL_SIZE);
    setFocusPolicy(_isBlack ? Qt::NoFocus : Qt::StrongFocus);
}

void CellWidget::setLetter(char c)
{
    if (_isBlack) return;
    _letter = (c == '.' || c == '\0') ? '_' : c;
    update();
}

void CellWidget::setBlack(bool black)
{
    _isBlack = black;
    _letter  = black ? '#' : '_';
    setFocusPolicy(black ? Qt::NoFocus : Qt::StrongFocus);
    update();
}

void CellWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (_isBlack) {
        p.fillRect(rect(), Qt::black);
    } else {
        // White fill
        p.fillRect(rect(), Qt::white);
        // Border
        p.setPen(QPen(QColor(80, 80, 80), 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // Focus ring
        if (hasFocus()) {
            p.setPen(QPen(QColor(0, 120, 215), 2));
            p.drawRect(rect().adjusted(1, 1, -2, -2));
        }

        // Letter
        if (_letter != '_') {
            QFont f = p.font();
            f.setPixelSize(CELL_SIZE * 14 / 20);
            f.setBold(true);
            p.setFont(f);
            p.setPen(Qt::black);
            p.drawText(rect(), Qt::AlignCenter, QString(QChar(std::toupper(_letter))));
        }
    }
}

void CellWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (!_isBlack) setFocus();
        emit clicked(this);
    }
    QWidget::mousePressEvent(event);
}

void CellWidget::keyPressEvent(QKeyEvent* event)
{
    if (_isBlack) { QWidget::keyPressEvent(event); return; }

    int key = event->key();
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        _letter = static_cast<char>('a' + (key - Qt::Key_A));
        update();
    } else if (key == Qt::Key_Backspace || key == Qt::Key_Delete || key == Qt::Key_Space) {
        _letter = '_';
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}

// ═══════════════════════════════════════════════════════════════
//  GridWidget
// ═══════════════════════════════════════════════════════════════

GridWidget::GridWidget(QWidget* parent) : QWidget(parent) {}

void GridWidget::loadFromGrid(const Grid& grid)
{
    // Clear existing cells
    for (auto& row : _cells)
        for (auto* cell : row)
            delete cell;
    _cells.clear();

    // Delete old layout if any
    if (layout()) {
        QLayoutItem* item;
        while ((item = layout()->takeAt(0)) != nullptr) delete item;
        delete layout();
    }

    _rows = grid.getRows();
    _cols = grid.getCols();
    _cells.resize(_rows, QVector<CellWidget*>(_cols, nullptr));

    auto* gl = new QGridLayout(this);
    gl->setSpacing(0);
    gl->setContentsMargins(0, 0, 0, 0);

    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            bool black = (grid.getValue(r, c) == '#');
            auto* cw = new CellWidget(black, this);
            if (!black) {
                char v = static_cast<char>(grid.getValue(r, c));
                if (v != '_' && v != '.') cw->setLetter(v);
            }
            connect(cw, &CellWidget::clicked, this, [this, cw](CellWidget*) {
                // Toggle black on right-click is handled via mousePressEvent
                Q_UNUSED(cw)
            });
            _cells[r][c] = cw;
            gl->addWidget(cw, r, c);
        }
    }

    setFixedSize(_cols * CellWidget::CELL_SIZE, _rows * CellWidget::CELL_SIZE);
}

void GridWidget::applySnapshot(const QVector<QVector<char>>& snapshot)
{
    for (int r = 0; r < _rows && r < snapshot.size(); ++r) {
        for (int c = 0; c < _cols && c < snapshot[r].size(); ++c) {
            if (!_cells[r][c]->isBlack()) {
                _cells[r][c]->setLetter(snapshot[r][c]);
            }
        }
    }
}

QVector<QVector<char>> GridWidget::toCharGrid() const
{
    QVector<QVector<char>> result(_rows, QVector<char>(_cols, '.'));
    for (int r = 0; r < _rows; ++r)
        for (int c = 0; c < _cols; ++c)
            result[r][c] = _cells[r][c]->isBlack() ? '#' : _cells[r][c]->letter();
    return result;
}

// ═══════════════════════════════════════════════════════════════
//  MainWindow
// ═══════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Hitz Gurutzatuak Solver");
    resize(900, 620);

    _dict = std::make_unique<Dict>();

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

    QAction* actNew = fileMenu->addAction(tr("&New Grid"));
    actNew->setShortcut(QKeySequence::New);
    connect(actNew, &QAction::triggered, this, &MainWindow::onNewGrid);

    QAction* actOpen = fileMenu->addAction(tr("&Open Grid…"));
    actOpen->setShortcut(QKeySequence::Open);
    connect(actOpen, &QAction::triggered, this, &MainWindow::onOpenGrid);

    QAction* actSave = fileMenu->addAction(tr("&Save Grid…"));
    actSave->setShortcut(QKeySequence::Save);
    connect(actSave, &QAction::triggered, this, &MainWindow::onSaveGrid);

    fileMenu->addSeparator();

    QAction* actQuit = fileMenu->addAction(tr("&Quit"));
    actQuit->setShortcut(QKeySequence::Quit);
    connect(actQuit, &QAction::triggered, qApp, &QApplication::quit);

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

// ── File slots ────────────────────────────────────────────────

void MainWindow::onNewGrid()
{
    stopSolver();
    // For now just reload the default; a dialog to pick dimensions could be added later
    loadDefaultGrid();
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
    auto grid = _gridWidget->toCharGrid();
    for (const auto& row : grid) {
        for (char c : row) out << QChar(c == '_' ? '.' : c);
        out << '\n';
    }
    statusBar()->showMessage(tr("Grid saved: %1").arg(path));
}

// ── Solver slots ─────────────────────────────────────────────

void MainWindow::onSolve()
{
    if (_solving) return;
    if (!_crossword) { QMessageBox::information(this, tr("Solver"), tr("No grid loaded.")); return; }

    Grid* grid = &_crossword->getGrid();

    // ── Camino 1: grid ya resuelto → reset y empezar de nuevo ──
    if (grid->isSolved()) {
        grid->reset();
        _gridWidget->loadFromGrid(*grid);   // refresca la UI al estado vacío
        statusBar()->showMessage(tr("Grid reseteado. Resolviendo desde cero…"));
        _statusLabel->setText(tr("Resolviendo desde cero…"));
    }
    // ── Camino 2: estado intermedio → continuar desde donde está ──
    else {
        statusBar()->showMessage(tr("Continuando desde el estado actual…"));
        _statusLabel->setText(tr("Continuando…"));
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
