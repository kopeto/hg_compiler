#include "puzuploaddialog.h"

#include "../clue.h"
#include "../grid.h"
#include "../grid_word.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

PuzUploadDialog::PuzUploadDialog(Grid& grid, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Esportatu PUZ-a zerbitzarira"));
    setModal(true);
    setMinimumWidth(580);
    setMinimumHeight(500);

    auto* form = new QFormLayout;

    // ── Server URL ─────────────────────────────────────────
    auto* serverRow   = new QHBoxLayout;
    auto* prefixLabel = new QLabel(QStringLiteral("https://"), this);
    prefixLabel->setStyleSheet("color: #555;");
    _serverEdit = new QLineEdit(this);
    _serverEdit->setPlaceholderText(tr("miserver.com"));
    auto* suffixLabel = new QLabel(QStringLiteral("/external/puzzle"), this);
    suffixLabel->setStyleSheet("color: #555;");
    serverRow->addWidget(prefixLabel);
    serverRow->addWidget(_serverEdit, 1);
    serverRow->addWidget(suffixLabel);
    form->addRow(tr("Zerbitzaria:"), serverRow);

    // ── API Key ────────────────────────────────────────────
    _apiKeyEdit = new QLineEdit(this);
    _apiKeyEdit->setPlaceholderText(tr("Bearer API gakoa"));
    _apiKeyEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("API Key:"), _apiKeyEdit);

    // ── Metadata fields ─────────────────────────────────────
    _titleEdit = new QLineEdit(this);
    _titleEdit->setPlaceholderText(tr("Kurtzearen izenburua"));
    form->addRow(tr("Izenburua:"), _titleEdit);

    _authorEdit = new QLineEdit(this);
    _authorEdit->setPlaceholderText(tr("Egilearen izena"));
    form->addRow(tr("Egilea:"), _authorEdit);

    _copyrightEdit = new QLineEdit(this);
    _copyrightEdit->setText(tr("© 2026 HitzGurutzatuak"));
    form->addRow(tr("Copyright:"), _copyrightEdit);

    // ── Clue table ───────────────────────────────────────────
    auto* clueLabel = new QLabel(tr("Pistak:"), this);
    clueLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");

    _clueTable = new QTableWidget(0, 3, this);
    _clueTable->setHorizontalHeaderLabels({tr("#"), tr("Hitza"), tr("Pista")});
    _clueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    _clueTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    _clueTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    _clueTable->horizontalHeader()->setStretchLastSection(true);
    _clueTable->verticalHeader()->setVisible(false);
    _clueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _clueTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed |
                                QAbstractItemView::AnyKeyPressed);
    _clueTable->setAlternatingRowColors(true);
    _clueTable->setShowGrid(false);
    _clueTable->setStyleSheet("QTableWidget { border: 1px solid #c0c0c0; border-radius: 4px; }"
                              "QTableWidget::item { padding: 4px 8px; }"
                              "QHeaderView::section { background: #f0f0f0; font-weight: bold; "
                              "                       padding: 4px 8px; border: none; "
                              "                       border-bottom: 1px solid #c0c0c0; }");
    buildClueTable(grid);

    // ── Buttons ─────────────────────────────────────────────
    _buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    _buttons->button(QDialogButtonBox::Ok)->setText(tr("Igo / Upload"));
    connect(_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // ── Main layout ─────────────────────────────────────────
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(clueLabel);
    mainLayout->addWidget(_clueTable, /*stretch=*/1);
    mainLayout->addWidget(_buttons);
}

// Helper: insert a non-editable group-header row spanning all columns
static void insertGroupHeader(QTableWidget* table, const QString& label) {
    const int row = table->rowCount();
    table->insertRow(row);
    auto* item = new QTableWidgetItem(label);
    item->setFlags(Qt::ItemIsEnabled);
    item->setBackground(QColor("#ddeeff"));
    QFont f = item->font();
    f.setBold(true);
    item->setFont(f);
    table->setItem(row, 0, item);
    table->setSpan(row, 0, 1, 3);
}

void PuzUploadDialog::buildClueTable(Grid& grid) {
    const int rows = grid.getRows();
    const int cols = grid.getCols();

    struct Entry {
        int  num;
        int  r;
        int  c;
        bool across;
        bool down;
    };
    std::vector<Entry> entries;
    int                num = 1;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (grid.getValue(r, c) == '#')
                continue;
            bool startsAcross =
                (c == 0 || grid.getValue(r, c - 1) == '#') && (c + 1 < cols && grid.getValue(r, c + 1) != '#');
            bool startsDown =
                (r == 0 || grid.getValue(r - 1, c) == '#') && (r + 1 < rows && grid.getValue(r + 1, c) != '#');
            if (startsAcross || startsDown) {
                entries.push_back({num++, r, c, startsAcross, startsDown});
            }
        }
    }

    auto addWordRow = [&](int wordNum, int r, int c, GridWordDirection dir) {
        GridWord* gw = nullptr;
        try {
            gw = grid.getGridWordAt(static_cast<unsigned>(r), static_cast<unsigned>(c), dir);
        } catch (...) {
        }
        if (!gw)
            return;

        const int tableRow = _clueTable->rowCount();
        _clueTable->insertRow(tableRow);

        auto* numItem = new QTableWidgetItem(QString::number(wordNum));
        numItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        numItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        _clueTable->setItem(tableRow, 0, numItem);

        QString wordStr;
        wordStr.reserve(static_cast<int>(gw->cells.size()));
        for (const Cell* cell : gw->cells)
            wordStr += QChar(cell->value == '_' ? '?' : static_cast<char>(std::toupper((unsigned char)cell->value)));
        auto* dirItem = new QTableWidgetItem(wordStr);
        dirItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        dirItem->setFont(QFont("Courier New", 9));
        _clueTable->setItem(tableRow, 1, dirItem);

        QString clueText;
        if (const Clue* clue = gw->getClue())
            clueText = QString::fromStdString(clue->getClueText());
        _clueTable->setItem(tableRow, 2, new QTableWidgetItem(clueText));

        _rowToWord.push_back(gw);
    };

    insertGroupHeader(_clueTable, tr("  Zehar"));
    _rowToWord.push_back(nullptr);
    for (const auto& e : entries)
        if (e.across)
            addWordRow(e.num, e.r, e.c, GridWordDirection::ACROSS);

    insertGroupHeader(_clueTable, tr("  Behera"));
    _rowToWord.push_back(nullptr);
    for (const auto& e : entries)
        if (e.down)
            addWordRow(e.num, e.r, e.c, GridWordDirection::DOWN);
}

void PuzUploadDialog::applyClues() const {
    for (int row = 0; row < static_cast<int>(_rowToWord.size()); ++row) {
        GridWord* gw = _rowToWord[row];
        if (!gw)
            continue;
        QTableWidgetItem* item = _clueTable->item(row, 2);
        if (!item)
            continue;
        gw->setClue(item->text().toStdString());
    }
}

QString PuzUploadDialog::serverUrl() const {
    QString host = _serverEdit->text().trimmed();
    if (host.isEmpty())
        return {};
    return QStringLiteral("https://") + host + QStringLiteral("/external/puzzle");
}
QString PuzUploadDialog::apiKey() const {
    return _apiKeyEdit->text().trimmed();
}
QString PuzUploadDialog::title() const {
    return _titleEdit->text().trimmed();
}
QString PuzUploadDialog::author() const {
    return _authorEdit->text().trimmed();
}
QString PuzUploadDialog::copyright() const {
    return _copyrightEdit->text().trimmed();
}
