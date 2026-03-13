#include "newgriddialog.h"

#include "../qt_styles.h"

#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

static constexpr int DEFAULT_ROWS = 8;
static constexpr int DEFAULT_COLS = 8;
static constexpr int MAX_PREVIEW  = 480; // px — max scroll area size

NewGridDialog::NewGridDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("New Blank Grid"));
    setModal(true);

    // ── Spinboxes ──────────────────────────────────────────
    auto* form = new QFormLayout;

    _rowsSpin = new QSpinBox(this);
    _rowsSpin->setRange(2, 30);
    _rowsSpin->setValue(DEFAULT_ROWS);
    form->addRow(tr("Rows:"), _rowsSpin);

    _colsSpin = new QSpinBox(this);
    _colsSpin->setRange(2, 30);
    _colsSpin->setValue(DEFAULT_COLS);
    form->addRow(tr("Columns:"), _colsSpin);

    // ── Hint label ─────────────────────────────────────────
    auto* hint = new QLabel(tr("Right-click a cell to toggle black / white"), this);
    hint->setStyleSheet(HG::Styles::kSmallMuted);

    // ── Preview (GridWidget inside a scroll area) ──────────
    _preview = new GridWidget(this);
    _preview->setEditMode(true);

    _scroll = new QScrollArea(this);
    _scroll->setWidget(_preview);
    _scroll->setWidgetResizable(false);
    _scroll->setMinimumSize(MAX_PREVIEW, MAX_PREVIEW);
    _scroll->setFrameShape(QFrame::StyledPanel);

    // ── Buttons ────────────────────────────────────────────
    _buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // ── Layout ─────────────────────────────────────────────
    auto* topRow = new QHBoxLayout;
    topRow->addLayout(form);
    topRow->addStretch();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topRow);
    mainLayout->addWidget(hint);
    mainLayout->addWidget(_scroll);
    mainLayout->addWidget(_buttons);

    // ── Wire spinbox changes → rebuild preview ─────────────
    connect(_rowsSpin, &QSpinBox::valueChanged, this, &NewGridDialog::rebuildPreview);
    connect(_colsSpin, &QSpinBox::valueChanged, this, &NewGridDialog::rebuildPreview);

    rebuildPreview();
}

void NewGridDialog::rebuildPreview() {
    _preview->loadBlank(_rowsSpin->value(), _colsSpin->value());
    // Let the scroll area re-evaluate the new fixed size
    _scroll->updateGeometry();
}

int NewGridDialog::gridRows() const {
    return _rowsSpin->value();
}
int NewGridDialog::gridCols() const {
    return _colsSpin->value();
}

QVector<QVector<char>> NewGridDialog::layout() const {
    return _preview->toCharGrid();
}
