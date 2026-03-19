#include "mainwindow.h"

#include "clue.h"
#include "eeh_worker.h"
#include "hg_config.h"
#include "paths.h"
#include "puz_serializer.h"
#include "qt_styles.h"
#include "ui/newgriddialog.h"
#include "ui/puzexportdialog.h"
#include "ui/puzuploaddialog.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslSocket>
#include <QStatusBar>
#include <QTextStream>
#include <QVBoxLayout>
#include <cctype>

// ═══════════════════════════════════════════════════════════════
//  MainWindow
// ═══════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Hitz Gurutzatuak");
    resize(900, 620);

    // Load persistent config first so later initialisations can use its values.
    _config.load();

    // Dict: use last-used path from config, fall back to default.
    _dictPath = _config.dictPath.isEmpty() ? HG::defaultDictPath() : _config.dictPath;
    _dict     = std::make_unique<Dict>(_dictPath.toStdString());

    // EEH database: open once at startup; non-fatal if missing.
    const QString dbPath = HG::defaultDbPath();
    // Start EEH worker thread and initialise DB there to avoid blocking UI
    _eehThread = new QThread(this);
    _eehWorker = new EehWorker();
    _eehWorker->moveToThread(_eehThread);
    connect(_eehThread, &QThread::finished, _eehWorker, &QObject::deleteLater);
    connect(this, &MainWindow::requestEehLookup, _eehWorker, &EehWorker::lookup, Qt::QueuedConnection);
    connect(_eehWorker, &EehWorker::lookupDone, this, &MainWindow::onEehLookupDone, Qt::QueuedConnection);
    _eehThread->start();
    // Initialise DB inside worker thread
    QMetaObject::invokeMethod(_eehWorker, "init", Qt::QueuedConnection, Q_ARG(QString, dbPath));

    _refreshTimer = new QTimer(this);
    _refreshTimer->setInterval(250); // 4 Hz
    connect(_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshTimer);

    setupMenuBar();
    setupCentralWidget();
    loadDefaultGrid();
}

MainWindow::~MainWindow() {
    forceStopSolver();
    if (_eehThread) {
        _eehThread->quit();
        _eehThread->wait();
        _eehThread = nullptr;
        _eehWorker = nullptr;
    }
}

// ── UI setup ─────────────────────────────────────────────────

void MainWindow::setupMenuBar() {
    // ── File ──
    QMenu* fileMenu = menuBar()->addMenu(tr("&Fitxategia"));

    QAction* actQuit = fileMenu->addAction(tr("&Irten"));
    actQuit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(actQuit, &QAction::triggered, qApp, &QApplication::quit);

    // Restored PUZ menu (previous line removed accidentally).
    QMenu* puzMenu = menuBar()->addMenu(tr("&PUZ"));

    QAction* actImport = puzMenu->addAction(tr("&Inportatu .puz…"));
    actImport->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(actImport, &QAction::triggered, this, &MainWindow::onImportPuz);

    QAction* actExport = puzMenu->addAction(tr("&Esportatu .puz gisa…"));
    actExport->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(actExport, &QAction::triggered, this, &MainWindow::onExportPuz);

    QAction* actUpload = puzMenu->addAction(tr("&Igo .puz zerbitzarira…"));
    actUpload->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_U));
    connect(actUpload, &QAction::triggered, this, &MainWindow::onUploadPuz);

    // ── Grid ──
    QMenu* gridMenu = menuBar()->addMenu(tr("&Koadroa"));

    QAction* actNewBlank = gridMenu->addAction(tr("&Koadro Berri Hutsua…"));
    actNewBlank->setShortcut(QKeySequence::New);
    connect(actNewBlank, &QAction::triggered, this, &MainWindow::onNewBlankGrid);

    QAction* actOpen = gridMenu->addAction(tr("&Ireki Koadroa…"));
    actOpen->setShortcut(QKeySequence::Open);
    connect(actOpen, &QAction::triggered, this, &MainWindow::onOpenGrid);

    QAction* actSave = gridMenu->addAction(tr("&Gorde Koadroa…"));
    actSave->setShortcut(QKeySequence::Save);
    connect(actSave, &QAction::triggered, this, &MainWindow::onSaveGrid);

    gridMenu->addSeparator();

    _actEditMode = gridMenu->addAction(tr("&Editatze modua"));
    _actEditMode->setCheckable(true);
    _actEditMode->setShortcut(Qt::Key_F2);
    _actEditMode->setToolTip(tr("Editatze modua (F2)\nKoadroko gelaxka beltzak gehitu/kentzeko."));
    connect(_actEditMode, &QAction::toggled, this, &MainWindow::onToggleEditMode);

    // ── Dictionary ──
    QMenu* dictMenu = menuBar()->addMenu(tr("&Hiztegia"));

    QAction* actDefDict = dictMenu->addAction(tr("Kargatu &Lehenetsitako Hiztegia"));
    actDefDict->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(actDefDict, &QAction::triggered, this, &MainWindow::onLoadDefaultDictionary);

    QAction* actCustDict = dictMenu->addAction(tr("Kargatu &Hiztegi Pertsonalizatua…"));
    actCustDict->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    connect(actCustDict, &QAction::triggered, this, &MainWindow::onLoadCustomDictionary);

    // ── Solver ──
    QMenu* solverMenu = menuBar()->addMenu(tr("&Konponketa"));

    _actSolve = solverMenu->addAction(tr("&Ebaztu"));
    _actSolve->setShortcut(Qt::Key_F5);
    connect(_actSolve, &QAction::triggered, this, &MainWindow::onSolve);

    _actStop = solverMenu->addAction(tr("&Pausatu (Esc)"));
    _actStop->setShortcut(Qt::Key_Escape);
    connect(_actStop, &QAction::triggered, this, &MainWindow::pauseSolver);

    _actResume = solverMenu->addAction(tr("&Jarraitu"));
    _actResume->setShortcut(Qt::Key_F6);
    _actResume->setEnabled(false);
    connect(_actResume, &QAction::triggered, this, &MainWindow::onResumeSolver);

    // ── Toolbar (below menu bar) ──────────────────────────────────────
    _toolbar = addToolBar(tr("Tresna-barra"));
    _toolbar->setMovable(false);
    _toolbar->setFloatable(false);
    _toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // Re-use the existing checkable action so menu and toolbar stay in sync
    _actEditMode->setIcon(QIcon(":/icons/edit_mode.svg"));
    _toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _toolbar->addAction(_actEditMode);

    _actSymmetry = new QAction(QIcon(":/icons/symmetry.svg"), tr("Simetria"), this);
    _actSymmetry->setCheckable(true);
    _actSymmetry->setToolTip(tr("Simetria (180\u00b0)\nGelaxka beltzak ardatz simetrikoan gehitu/kentzeko."));
    _actSymmetry->setEnabled(false); // only active in edit mode
    // Connection to _gridWidget done in setupCentralWidget() after it is created
    _toolbar->addAction(_actSymmetry);
}

void MainWindow::setupCentralWidget() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout    = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);

    // ── Left: container that centres the grid ──
    _gridWidget = new GridWidget(this);
    connect(_gridWidget, &GridWidget::gridModified, this, &MainWindow::onGridModified);
    connect(_gridWidget, &GridWidget::cellFixed, this, &MainWindow::onCellFixed);
    connect(_gridWidget, &GridWidget::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(_gridWidget, &GridWidget::interactionRequested, this, &MainWindow::pauseSolver, Qt::DirectConnection);
    // Symmetry action created in setupMenuBar(); connect now that _gridWidget exists
    connect(_actSymmetry, &QAction::toggled, _gridWidget, &GridWidget::setSymmetry);

    _gridArea = new QWidget(this);
    _gridArea->setStyleSheet(HG::Styles::kGridArea);
    auto* gridAreaLayout = new QVBoxLayout(_gridArea);
    gridAreaLayout->setContentsMargins(8, 8, 8, 8);
    gridAreaLayout->setSpacing(6);

    // Grid: centred horizontally, pushes to top, no vertical stretch
    gridAreaLayout->addStretch(1);
    gridAreaLayout->addWidget(_gridWidget, 0, Qt::AlignHCenter);
    gridAreaLayout->addStretch(1);

    // ── Bottom panel — metadata fields + controls ──────────────
    auto* bottomPanel = new QFrame(_gridArea);
    bottomPanel->setFrameShape(QFrame::NoFrame);
    auto* bottomLayout = new QVBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(4, 6, 4, 2);
    bottomLayout->setSpacing(5);

    // ── Metadata row ──
    auto* metaRow = new QHBoxLayout;
    metaRow->setSpacing(6);

    auto makeMetaLabel = [&](const QString& text) {
        auto* lbl = new QLabel(text, bottomPanel);
        lbl->setStyleSheet(HG::Styles::kSmallLabel);
        return lbl;
    };

    metaRow->addWidget(makeMetaLabel(tr("Izenburua:")));
    _metaTitleEdit = new QLineEdit(bottomPanel);
    _metaTitleEdit->setPlaceholderText(tr("Kurtzearen izenburua"));
    metaRow->addWidget(_metaTitleEdit, 2);

    metaRow->addWidget(makeMetaLabel(tr("Egilea:")));
    _metaAuthorEdit = new QLineEdit(bottomPanel);
    _metaAuthorEdit->setPlaceholderText(tr("Egilearen izena"));
    metaRow->addWidget(_metaAuthorEdit, 2);

    metaRow->addWidget(makeMetaLabel(tr("Copyright:")));
    _metaCopyrightEdit = new QLineEdit(bottomPanel);
    _metaCopyrightEdit->setPlaceholderText(tr("\u00a9 2026 HitzGurutzatuak"));
    metaRow->addWidget(_metaCopyrightEdit, 3);

    bottomLayout->addLayout(metaRow);

    // Connect edits → crossword metadata (live update)
    connect(_metaTitleEdit, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (_crossword)
            _crossword->title = t.toStdString();
    });
    connect(_metaAuthorEdit, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (_crossword)
            _crossword->author = t.toStdString();
        _config.author = t;
        _config.save();
    });
    connect(_metaCopyrightEdit, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (_crossword)
            _crossword->copyright = t.toStdString();
    });

    // ── Button row ──
    auto* btnRow    = new QWidget(bottomPanel);
    auto* btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(6);

    auto makeBtn = [&](const QString& text, const QString& tooltip) -> QPushButton* {
        auto* btn = new QPushButton(text, btnRow);
        btn->setToolTip(tooltip);
        btn->setStyleSheet(HG::Styles::kPushButton);
        return btn;
    };

    _resumeButton = makeBtn(tr("\u25b6  Jarraitu"), tr("Ebazlea jarraitu (F6)"));
    _resumeButton->setVisible(false);
    connect(_resumeButton, &QPushButton::clicked, this, &MainWindow::onResumeSolver);
    btnLayout->addWidget(_resumeButton);

    _clearButton = makeBtn(tr("Koadroa Garbitu"), tr("Kendu letra guztiak, gelaxka beltzak utzita"));
    connect(_clearButton, &QPushButton::clicked, this, &MainWindow::onClearGrid);
    btnLayout->addWidget(_clearButton);

    btnLayout->addStretch();

    bottomLayout->addWidget(btnRow);

    gridAreaLayout->addWidget(bottomPanel, 0); // fixed height, always below the grid

    mainLayout->addWidget(_gridArea, /*stretch=*/3);

    // ── Right panel ──
    auto* rightPanel = new QFrame(this);
    rightPanel->setFrameShape(QFrame::StyledPanel);
    rightPanel->setMinimumWidth(200);

    auto* rightLayout = new QVBoxLayout(rightPanel);

    // Removed UI: dictionary label and edit-mode label were deleted from right panel

    _statusLabel = new QLabel(tr("Prest"), rightPanel);
    _statusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    _statusLabel->setWordWrap(true);
    rightLayout->addWidget(_statusLabel);

    // Word list: shows candidates for the currently selected word
    _wordListLabel = new QLabel(tr("Hautagaiak:"), rightPanel);
    _wordListLabel->setStyleSheet(HG::Styles::kSmallBold);
    rightLayout->addWidget(_wordListLabel);

    _wordList = new QListWidget(rightPanel);
    _wordList->setAlternatingRowColors(true);
    _wordList->setStyleSheet(HG::Styles::kWordList);
    connect(_wordList, &QListWidget::itemDoubleClicked, this, &MainWindow::onWordListDoubleClicked);
    connect(_wordList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item)
            lookupWord(item->text());
    });
    rightLayout->addWidget(_wordList, /*stretch=*/1);

    auto* clueLabel = new QLabel(tr("Pista:"), rightPanel);
    clueLabel->setStyleSheet(HG::Styles::kSectionLabel);
    rightLayout->addWidget(clueLabel);

    _clueEdit = new QTextEdit(rightPanel);
    _clueEdit->setPlaceholderText(tr("Idatzi pista hemen\u2026"));
    _clueEdit->setEnabled(false);
    _clueEdit->setAcceptRichText(false);
    _clueEdit->setMinimumHeight(_clueEdit->fontMetrics().lineSpacing() * 4 + 12);
    rightLayout->addWidget(_clueEdit);
    connect(_clueEdit, &QTextEdit::textChanged, this, &MainWindow::onClueChanged);

    mainLayout->addWidget(rightPanel, /*stretch=*/1);

    // ── Info panel: definitions & examples (EEH) ──
    auto* infoPanel = new QFrame(this);
    infoPanel->setFrameShape(QFrame::StyledPanel);
    infoPanel->setMinimumWidth(280);
    auto* infoLayout = new QVBoxLayout(infoPanel);
    infoLayout->setContentsMargins(8, 8, 8, 8);
    _eehBrowser = new QTextBrowser(infoPanel);
    _eehBrowser->setOpenExternalLinks(true);
    _eehBrowser->setPlaceholderText(tr("Definizioak eta adibideak hemen agertuko dira..."));
    infoLayout->addWidget(_eehBrowser, /*stretch=*/1);
    mainLayout->addWidget(infoPanel, /*stretch=*/0);

    setCentralWidget(centralWidget);
    statusBar()->showMessage(tr("Prest"));
}

// ── Domain helpers ────────────────────────────────────────────

void MainWindow::syncMetaToWidgets() {
    if (!_metaTitleEdit)
        return;
    auto syncEdit = [](QLineEdit* ed, const std::string& val) {
        ed->blockSignals(true);
        ed->setText(QString::fromStdString(val));
        ed->blockSignals(false);
    };
    if (_crossword) {
        syncEdit(_metaTitleEdit, _crossword->title);
        // If the crossword has no author, seed from last-used config value.
        std::string authorVal = _crossword->author.empty() ? _config.author.toStdString() : _crossword->author;
        syncEdit(_metaAuthorEdit, authorVal);
        if (_crossword->author.empty())
            _crossword->author = authorVal;
        syncEdit(_metaCopyrightEdit, _crossword->copyright);
    } else {
        syncEdit(_metaTitleEdit, {});
        syncEdit(_metaAuthorEdit, _config.author.toStdString());
        syncEdit(_metaCopyrightEdit, {});
    }
}

void MainWindow::loadDefaultGrid() {
    try {
        _currentGridPath = HG::defaultGridPath().toStdString();
        _crossword       = std::make_unique<Crossword>(_currentGridPath);

        _gridWidget->loadFromGrid(_crossword->getGrid());
        adjustWindowForGrid();
        syncMetaToWidgets();
        // Always start in edit mode
        _actEditMode->setChecked(true);
        statusBar()->showMessage(tr("Lehenetsitako koadroa kargatu da."));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Kargatze errorea"), QString::fromStdString(e.what()));
    }
}

void MainWindow::onCellFixed(int row, int col, char letter, bool fixed) {
    if (!_crossword)
        return;
    Grid& g = _crossword->getGrid();
    if (fixed) {
        g.fixCell(row, col, letter);
    } else {
        g.unfixCell(row, col);
    }

    // Refresh the candidate list — letters changed
    updateWordList(_gridWidget->selectedRow(), _gridWidget->selectedCol(), _gridWidget->selectedDir());
}

void MainWindow::onSelectionChanged(int row, int col, GridWordDirection dir) {
    updateWordList(row, col, dir);
}

void MainWindow::updateWordList(int row, int col, GridWordDirection dir) {
    if (!_wordList)
        return;
    _wordList->clear();

    // Resolve the GridWord for this cell (independent of dict)
    GridWord* gw = nullptr;
    if (_crossword && row >= 0) {
        try {
            gw = _crossword->getGrid().getGridWordAt(static_cast<unsigned>(row), static_cast<unsigned>(col), dir);
        } catch (...) {
            gw = nullptr;
        }
    }

    // ── Clue edit: enable/populate whenever we have a GridWord ──
    if (_clueEdit) {
        if (gw) {
            Clue* clue = gw->getClue();
            _clueEdit->blockSignals(true);
            _clueEdit->setPlainText(clue ? QString::fromStdString(clue->getClueText()) : QString());
            _clueEdit->blockSignals(false);
            _clueEdit->setEnabled(true);
        } else {
            _clueEdit->blockSignals(true);
            _clueEdit->clear();
            _clueEdit->blockSignals(false);
            _clueEdit->setEnabled(false);
        }
    }

    // ── Candidate list: also needs a dict ──
    if (!_crossword || !_dict || !gw || row < 0) {
        _wordListLabel->setText(tr("Hautagaiak:"));
        return;
    }

    // Build the pattern from ALL visible letters in the word:
    //   any cell with a letter (fixed or not) → that letter (hard constraint)
    //   empty cell ('_')                       → '_' (wildcard)
    std::string patStr;
    patStr.reserve(gw->length);
    bool complete = true;
    for (const Cell* cell : gw->cells) {
        char v = static_cast<char>(std::toupper((unsigned char)cell->value));
        patStr += (v != '_') ? v : '_';
        if (v == '_')
            complete = false;
    }

    if (complete)
        lookupWord(QString::fromStdString(patStr));

    Pattern                  pat(patStr);
    std::vector<const Word*> candidates = _dict->getWordsByPattern(pat);

    _wordListLabel->setText(tr("Hautagaiak (%1):").arg(candidates.size()));

    for (const Word* w : candidates) {
        auto* item = new QListWidgetItem(QString::fromStdString(w->str));
        _wordList->addItem(item);
    }
}

void MainWindow::onWordListDoubleClicked(QListWidgetItem* item) {
    if (!item || !_crossword)
        return;

    int               row = _gridWidget->selectedRow();
    int               col = _gridWidget->selectedCol();
    GridWordDirection dir = _gridWidget->selectedDir();
    if (row < 0)
        return;

    Grid&     g  = _crossword->getGrid();
    GridWord* gw = g.getGridWordAt(row, col, dir);
    if (!gw)
        return;

    std::string word = item->text().toStdString();
    if (word.size() != gw->length)
        return;

    // Write each letter into the grid model and the UI
    auto [startR, startC] = gw->getPosition();
    for (int i = 0; i < (int)word.size(); ++i) {
        int  r  = (dir == GridWordDirection::ACROSS) ? startR : startR + i;
        int  c  = (dir == GridWordDirection::ACROSS) ? startC + i : startC;
        char ch = static_cast<char>(std::toupper((unsigned char)word[i]));
        g.fixCell(r, c, ch);
        _gridWidget->setCellFixed(r, c, ch);
    }

    // Refresh candidate list to reflect the new (fully fixed) pattern
    updateWordList(row, col, dir);
}

void MainWindow::lookupWord(const QString& word) {
    // Forward the lookup request to the worker thread (non-blocking)
    emit requestEehLookup(word);
}

void MainWindow::onEehLookupDone(const QString& word, bool found, const QStringList& defs,
                                 const QStringList& examples) {
    if (!_eehBrowser)
        return;

    QString html;
    html += QString("<h2>%1</h2>").arg(word.toHtmlEscaped());
    if (!found || defs.isEmpty()) {
        html += QString("<p><i>%1</i></p>").arg(tr("Ez da definiziorik aurkitu."));
        _eehBrowser->setHtml(html);
        return;
    }

    html += "<div>";
    html += "<h3>Definitions</h3>";
    html += "<ol>";
    for (const QString& d : defs)
        html += QString("<li>%1</li>").arg(d.toHtmlEscaped());
    html += "</ol>";

    if (!examples.isEmpty()) {
        html += "<h3>Adibideak (Examples)</h3>";
        html += "<ul>";
        for (const QString& ex : examples)
            html += QString("<li>%1</li>").arg(ex.toHtmlEscaped());
        html += "</ul>";
    }
    html += "</div>";

    _eehBrowser->setHtml(html);
}

void MainWindow::onClueChanged() {
    if (!_crossword)
        return;
    int               row = _gridWidget->selectedRow();
    int               col = _gridWidget->selectedCol();
    GridWordDirection dir = _gridWidget->selectedDir();
    if (row < 0)
        return;
    GridWord* gw = nullptr;
    try {
        gw = _crossword->getGrid().getGridWordAt(static_cast<unsigned>(row), static_cast<unsigned>(col), dir);
    } catch (...) {
        return;
    }
    if (!gw)
        return;
    std::string text = _clueEdit->toPlainText().toStdString();
    if (Clue* clue = gw->getClue()) {
        clue->setClueText(text);
    } else {
        gw->setClue(Clue(text));
    }
}

// ── Grid slots ────────────────────────────────────────────────

void MainWindow::onNewBlankGrid() {
    forceStopSolver();
    NewGridDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    // Convert the dialog's char layout → vector<string> for Grid constructor
    auto                     charLayout = dlg.layout();
    std::vector<std::string> lines;
    lines.reserve(charLayout.size());
    for (const auto& row : charLayout) {
        std::string line;
        line.reserve(row.size());
        for (char c : row)
            line += (c == '#' ? '#' : '.');
        lines.push_back(line);
    }

    try {
        _crossword = std::make_unique<Crossword>(lines);
        _currentGridPath.clear();
        _gridWidget->loadFromGrid(_crossword->getGrid());
        adjustWindowForGrid();
        syncMetaToWidgets();
        _actEditMode->setChecked(true);
        statusBar()->showMessage(tr("Sortu da %1×%2 koadro berria. Editatze modua gaituta.")
                                     .arg(charLayout.size())
                                     .arg(charLayout.isEmpty() ? 0 : charLayout[0].size()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Errorea"), QString::fromStdString(e.what()));
    }
}

void MainWindow::onToggleEditMode(bool checked) {
    _gridWidget->setEditMode(checked);
    if (_actSymmetry)
        _actSymmetry->setEnabled(checked);
    updateEditModeIndicator();
    statusBar()->showMessage(
        checked ? tr("Editatze modua gaituta — egin klik eskuineko botoiarekin gelaxka beltz/zuri bihurtzeko")
                : tr("Editatze modua desgaituta"),
        3000);
}

void MainWindow::updateEditModeIndicator() {
    // Edit mode indicator removed from UI; keep function for compatibility.
    Q_UNUSED(_gridWidget);
}

void MainWindow::onOpenGrid() {
    forceStopSolver();
    QString path =
        QFileDialog::getOpenFileName(this, tr("Koadroa Ireki"), QString(), tr("Koadro fitxategiak (*.grid);;Fitxategi guztiak (*)"));
    if (path.isEmpty())
        return;

    try {
        _currentGridPath = path.toStdString();
        _crossword       = std::make_unique<Crossword>(_currentGridPath);
        _gridWidget->loadFromGrid(_crossword->getGrid());
        adjustWindowForGrid();
        syncMetaToWidgets();
        _actEditMode->setChecked(true);
        statusBar()->showMessage(tr("Koadroa kargatu da: %1").arg(path));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Kargatze errorea"), QString::fromStdString(e.what()));
    }
}

void MainWindow::onSaveGrid() {
    QString path =
        QFileDialog::getSaveFileName(this, tr("Gorde Koadroa"), QString(), tr("Koadro fitxategiak (*.grid);;Fitxategi guztiak (*)"));
    if (path.isEmpty())
        return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Gorde errorea"), tr("Ezin da fitxategia idazteko irekitu."));
        return;
    }
    QTextStream out(&f);

    // Save the current black/white layout (letters become '.' — grid format only
    // encodes structure, not solution)
    auto grid = _gridWidget->toCharGrid();
    for (const auto& row : grid) {
        for (char c : row)
            out << QChar(c == '#' ? '#' : '.');
        out << '\n';
    }
    statusBar()->showMessage(tr("Koadroa gorde da: %1").arg(path));
}

// ── Export .puz ─────────────────────────────────────────────

void MainWindow::onExportPuz() {
    if (!_crossword) {
        QMessageBox::warning(this, tr("Esportatu"), tr("Ez dago koadrorik kargatuta."));
        return;
    }

    PuzExportDialog dlg(_crossword->getGrid(), this);
    dlg.prefillMetadata(QString::fromStdString(_crossword->title), QString::fromStdString(_crossword->author),
                        QString::fromStdString(_crossword->copyright));
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString path = dlg.filePath();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Esportatu"), tr("Ez da fitxategi-bidarik hautatu."));
        return;
    }

    // Write edits from the dialog table back to the GridWord objects
    dlg.applyClues();

    QString err = PuzSerializer::exportToFile(_crossword->getGrid(), path, dlg.title().toStdString(),
                                              dlg.author().toStdString(), dlg.copyright().toStdString());

    if (!err.isEmpty())
        QMessageBox::critical(this, tr("Esportazio errorea"), err);
    else
        statusBar()->showMessage(tr("Esportatu da: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::onUploadPuz() {
    if (!_crossword) {
        QMessageBox::warning(this, tr("Igo"), tr("Ez dago koadrorik kargatuta."));
        return;
    }

    PuzUploadDialog dlg(_crossword->getGrid(), this);
    dlg.prefillMetadata(QString::fromStdString(_crossword->title), QString::fromStdString(_crossword->author),
                        QString::fromStdString(_crossword->copyright));
    dlg.prefillConnection(_config.serverHost, _config.apiKey);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString serverUrl = dlg.serverUrl();
    QString apiKey    = dlg.apiKey();
    if (serverUrl.isEmpty()) {
        QMessageBox::warning(this, tr("Igo"), tr("Zerbitzariaren URLa falta da."));
        return;
    }

    QUrl url(serverUrl);
    if (!url.isValid()) {
        QMessageBox::warning(this, tr("Igo"), tr("URLa ez da baliozkoa: %1").arg(url.errorString()));
        return;
    }

    if (!QSslSocket::supportsSsl()) {
        QMessageBox::critical(this, tr("Igo errorea"),
                              tr("HTTPS ez dago erabilgarri: SSL/TLS backend-a falta da.\n"
                                 "Ziurtatu Qt TLS pluginak (qopensslbackend, qschannelbackend) eskuragarri daudela."));
        return;
    }

    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("Igo"), tr("API Key falta da."));
        return;
    }

    // Persist connection settings for next session.
    _config.serverHost = dlg.serverHostOnly();
    _config.apiKey     = apiKey;
    _config.save();

    dlg.applyClues();

    QByteArray puzBytes = PuzSerializer::exportToBytes(_crossword->getGrid(), dlg.title().toStdString(),
                                                       dlg.author().toStdString(), dlg.copyright().toStdString());

    if (puzBytes.isEmpty()) {
        QMessageBox::critical(this, tr("Igo errorea"), tr("Ezin izan da .puz sortu."));
        return;
    }

    // Build multipart/form-data request (equivalent to curl -F "filename=@file.puz")
    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QStringLiteral("form-data; name=\"filename\"; filename=\"puzzle.puz\"")));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/octet-stream")));
    filePart.setBody(puzBytes);
    multiPart->append(filePart);

    QNetworkRequest request{url};
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(apiKey).toUtf8());

    auto*          manager = new QNetworkAccessManager(this);
    QNetworkReply* reply   = manager->post(request, multiPart);
    multiPart->setParent(reply); // ensure multiPart is deleted with reply

    statusBar()->showMessage(tr("Igotzen…"));

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        reply->deleteLater();
        manager->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(this, tr("Igo errorea"), tr("Errorea igotzen:\n%1").arg(reply->errorString()));
            statusBar()->showMessage(tr("Igoera huts egin du"), 5000);
        } else {
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (httpStatus >= 200 && httpStatus < 300) {
                statusBar()->showMessage(tr("Igoera ondo (HTTP %1)").arg(httpStatus), 5000);
            } else {
                QString body = QString::fromUtf8(reply->readAll()).left(500);
                QMessageBox::warning(this, tr("Igo"),
                                     tr("Zerbitzariak HTTP %1 erantzun du.\n%2").arg(httpStatus).arg(body));
                statusBar()->showMessage(tr("Igoera: HTTP %1").arg(httpStatus), 5000);
            }
        }
    });
}

void MainWindow::onImportPuz() {
    QString path = QFileDialog::getOpenFileName(this, tr("Inportatu .puz"), QString(), tr("Across Lite (*.puz)"));
    if (path.isEmpty())
        return;

    PuzData puzData = PuzSerializer::importFromFile(path);
    if (!puzData.errorMessage.isEmpty()) {
        QMessageBox::critical(this, tr("Inportazio errorea"), puzData.errorMessage);
        return;
    }

    auto lines = PuzSerializer::toGridLines(puzData);

    forceStopSolver();
    _crossword            = std::make_unique<Crossword>(lines);
    _crossword->title     = puzData.title;
    _crossword->author    = puzData.author;
    _crossword->copyright = puzData.copyright.empty() ? "\u00a9 2026 HitzGurutzatuak" : puzData.copyright;
    _currentGridPath.clear();

    // Mark imported letters as fixed so the solver preserves them
    {
        Grid& g = _crossword->getGrid();
        for (int r = 0; r < g.getRows(); ++r)
            for (int c = 0; c < g.getCols(); ++c) {
                char v = static_cast<char>(g.getValue(r, c));
                if (v != '#' && v != '_')
                    g.fixCell(r, c, v);
            }
    }

    _gridWidget->loadFromGrid(_crossword->getGrid());
    adjustWindowForGrid();
    syncMetaToWidgets();

    // Map the imported clue list onto the GridWords (same reading order as export)
    {
        Grid&     g    = _crossword->getGrid();
        const int rows = g.getRows();
        const int cols = g.getCols();
        int       ci   = 0;
        for (int r = 0; r < rows && ci < static_cast<int>(puzData.clues.size()); ++r) {
            for (int c = 0; c < cols && ci < static_cast<int>(puzData.clues.size()); ++c) {
                if (g.getValue(r, c) == '#')
                    continue;
                bool startsAcross =
                    (c == 0 || g.getValue(r, c - 1) == '#') && (c + 1 < cols && g.getValue(r, c + 1) != '#');
                bool startsDown =
                    (r == 0 || g.getValue(r - 1, c) == '#') && (r + 1 < rows && g.getValue(r + 1, c) != '#');
                if (startsAcross) {
                    try {
                        if (GridWord* gw = g.getGridWordAt(static_cast<unsigned>(r), static_cast<unsigned>(c),
                                                           GridWordDirection::ACROSS))
                            gw->setClue(Clue(puzData.clues[ci]));
                    } catch (...) {
                    }
                    ++ci;
                }
                if (startsDown && ci < static_cast<int>(puzData.clues.size())) {
                    try {
                        if (GridWord* gw = g.getGridWordAt(static_cast<unsigned>(r), static_cast<unsigned>(c),
                                                           GridWordDirection::DOWN))
                            gw->setClue(Clue(puzData.clues[ci]));
                    } catch (...) {
                    }
                    ++ci;
                }
            }
        }
    }

    updateWordList(-1, -1, GridWordDirection::ACROSS);
    statusBar()->showMessage(tr("Inportatu da: %1").arg(QFileInfo(path).fileName()));
}

// ── Dictionary slots ─────────────────────────────────────────

void MainWindow::onClearGrid() {
    if (!_crossword)
        return;
    forceStopSolver();

    Grid& g = _crossword->getGrid();

    // Unfix every cell and reset values to '_'
    for (int r = 0; r < g.getRows(); ++r)
        for (int c = 0; c < g.getCols(); ++c)
            g.unfixCell(r, c);
    g.reset(); // clears all non-fixed (now all) fillable cells to '_'

    _gridWidget->loadFromGrid(g);
    // Restore edit mode (loadFromGrid resets the widget state)
    _actEditMode->setChecked(true);
    _wordList->clear();
    _wordListLabel->setText(tr("Hautagaiak:"));
    statusBar()->showMessage(tr("Koadroa garbitu da."));
}

void MainWindow::onLoadDefaultDictionary() {
    try {
        _dictPath        = HG::defaultDictPath();
        _dict            = std::make_unique<Dict>(_dictPath.toStdString());
        _config.dictPath = "";
        _config.save();
            statusBar()->showMessage(tr("Lehenetsitako hiztegia kargatu da."), 3000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Hiztegi errorea"), tr("Ezin izan da hiztegia kargatu:\n%1").arg(e.what()));
        _dict = nullptr;
    }
    updateDictLabel();
}

void MainWindow::onLoadCustomDictionary() {
    QString path =
        QFileDialog::getOpenFileName(this, tr("Hautatu Hiztegia"), QString(), tr("Testu fitxategiak (*.txt);;Fitxategi guztiak (*)"));
    if (path.isEmpty())
        return;

    try {
        _dictPath = path;
        _dict     = std::make_unique<Dict>();
        _dict->load(path.toStdString());
        _config.dictPath = path;
        _config.save();
        statusBar()->showMessage(tr("Hiztegia kargatu da: %1").arg(path), 3000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Hiztegi errorea"), tr("Ezin izan da hiztegia kargatu:\n%1").arg(e.what()));
        _dict = nullptr;
    }
    updateDictLabel();
}

void MainWindow::updateDictLabel() {
    // Dictionary label removed from UI; no-op to preserve external calls.
    Q_UNUSED(_dict);
    Q_UNUSED(_dictPath);
}

void MainWindow::onGridModified() {
    if (!_gridWidget->editMode())
        return;

    // Snapshot fixed cells BEFORE rebuilding (loadFromGrid resets everything)
    auto fixed = _gridWidget->getFixedCells();

    auto                     charLayout = _gridWidget->toCharGrid();
    std::vector<std::string> lines;
    lines.reserve(charLayout.size());
    for (const auto& row : charLayout) {
        std::string line;
        line.reserve(row.size());
        for (char c : row)
            line += (c == '#' ? '#' : '.');
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
                if (!g.cellHasWord(fc.row, fc.col))
                    continue;
                _gridWidget->setCellFixed(fc.row, fc.col, fc.letter);
                g.fixCell(fc.row, fc.col, fc.letter);
            }

            statusBar()->showMessage(
                tr("Koadroa eguneratu da — %1 zeharkako, %2 beherako").arg(g.getAcrossWords().size()).arg(g.getDownWords().size()),
                2000);

            // Refresh candidate list — word structure may have changed
            updateWordList(_gridWidget->selectedRow(), _gridWidget->selectedCol(), _gridWidget->selectedDir());
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

void MainWindow::adjustWindowForGrid() {
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
    int newW = qMax(width(), minW);
    int newH = qMax(height(), minH + extraH);
    if (newW != width() || newH != height())
        resize(newW, newH);
}

void MainWindow::updateSolverActions() {
    if (_actSolve)
        _actSolve->setEnabled(!_solving);
    if (_actStop)
        _actStop->setEnabled(_solving);
    if (_actResume)
        _actResume->setEnabled(!_solving && _paused);
    if (_resumeButton)
        _resumeButton->setVisible(!_solving && _paused);
}

void MainWindow::pauseSolver() {
    if (!_solving)
        return;
    _paused = true;
    if (_solverWorker)
        _solverWorker->requestCancel();
    // Never wait() here — would deadlock the UI thread.
    // The solver fires onSolverFinished via QueuedConnection when done.
}

void MainWindow::onSolve() {
    if (_solving)
        return;
    if (!_crossword) {
        QMessageBox::information(this, tr("Konponketa"), tr("Ez dago koadrorik kargatuta."));
        return;
    }

    Grid* grid = &_crossword->getGrid();

    // Always reset non-fixed cells before a fresh solve so stale solver
    // letters from a previous run don't corrupt the pattern matching.
    grid->reset();
    _gridWidget->loadFromGrid(*grid);

    statusBar()->showMessage(tr("Ebazten…"));
    _statusLabel->setText(tr("⏳ Ebazten…"));

    _solving = true;
    _paused  = false;
    updateSolverActions();

    _solverThread = new QThread(this);
    _solverWorker = new SolverWorker(grid, _dict.get());
    _solverWorker->moveToThread(_solverThread);

    connect(_solverThread, &QThread::started, _solverWorker, &SolverWorker::run);
    connect(_solverWorker, &SolverWorker::finished, this, &MainWindow::onSolverFinished);

    _refreshTimer->start();
    _solverThread->start();
}

void MainWindow::onResumeSolver() {
    if (_solving || !_paused)
        return;
    if (!_crossword)
        return;

    _paused  = false;
    _solving = true;
    updateSolverActions();

    Grid* grid = &_crossword->getGrid();
    // Do NOT reset — resume from the exact state the solver left off.
    statusBar()->showMessage(tr("Konponketa jarraitu da"));
    _statusLabel->setText(tr("⏳ Ebazten…"));

    _solverThread = new QThread(this);
    _solverWorker = new SolverWorker(grid, _dict.get());
    _solverWorker->moveToThread(_solverThread);

    connect(_solverThread, &QThread::started, _solverWorker, &SolverWorker::run);
    connect(_solverWorker, &SolverWorker::finished, this, &MainWindow::onSolverFinished);

    _refreshTimer->start();
    _solverThread->start();
}

void MainWindow::onStopSolver() {
    if (!_solving)
        return;
    _paused           = false;
    _pendingAfterStop = nullptr;
    if (_solverWorker)
        _solverWorker->requestCancel();
    updateSolverActions();
    statusBar()->showMessage(tr("Konponketa gelditzen…"));
}

void MainWindow::onSolverFinished(bool success) {
    _refreshTimer->stop();
    _solving = false;

    // Thread has finished run() — safe to quit/wait now (returns immediately)
    if (_solverThread) {
        _solverThread->quit();
        _solverThread->wait();
        _solverThread->deleteLater();
        _solverThread = nullptr;
    }
    if (_solverWorker) {
        _solverWorker->deleteLater();
        _solverWorker = nullptr;
    }

    // Run any deferred action (e.g. grid rebuild triggered during solve)
    if (_pendingAfterStop) {
        auto action       = std::move(_pendingAfterStop);
        _pendingAfterStop = nullptr;
        action();
        updateSolverActions();
        return;
    }

    if (_paused) {
        // Keep the UI exactly as it was when the solver stopped — don't refresh.
        updateSolverActions();
        statusBar()->showMessage(tr("Konponketa geldirik — sakatu F6 edo Jarraitu jarraitzeko."));
        _statusLabel->setText(tr("⏸ Pausatu"));
    } else {
        // Final UI snapshot only when finishing naturally (not paused)
        onRefreshTimer();

        if (success) {
            _paused = false;
            updateSolverActions();
            statusBar()->showMessage(tr("Irtenbidea aurkitu da!"));
            _statusLabel->setText(tr("✅ Irtenbidea aurkitu da!"));
        } else {
            _paused = false;
            updateSolverActions();
            statusBar()->showMessage(tr("Ez da irtenbiderik aurkitu."));
            _statusLabel->setText(tr("❌ Ez da irtenbiderik aurkitu."));
        }
    }
}

void MainWindow::onRefreshTimer() {
    if (!_crossword)
        return;
    const Grid& g    = _crossword->getGrid();
    int         rows = g.getRows();
    int         cols = g.getCols();

    QVector<QVector<char>> snap(rows, QVector<char>(cols));
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            snap[r][c] = static_cast<char>(g.getValue(r, c));

    _gridWidget->applySnapshot(snap);
}

void MainWindow::forceStopSolver() {
    // Safe to block here: only called from destructor or before a new grid load,
    // never from inside a Qt signal delivery.
    _refreshTimer->stop();
    if (_solverWorker)
        _solverWorker->requestCancel();
    if (_solverThread) {
        _solverThread->quit();
        _solverThread->wait();
        _solverThread->deleteLater();
        _solverThread = nullptr;
    }
    if (_solverWorker) {
        _solverWorker->deleteLater();
        _solverWorker = nullptr;
    }
    _solving          = false;
    _pendingAfterStop = nullptr;
}
