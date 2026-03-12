#include "puzexportdialog.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

PuzExportDialog::PuzExportDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Esportatu .puz gisa"));
    setModal(true);
    setMinimumWidth(440);

    auto* form = new QFormLayout;

    // ── File path row ──────────────────────────────────────
    auto* pathRow    = new QHBoxLayout;
    _pathEdit        = new QLineEdit(this);
    _pathEdit->setPlaceholderText(tr("Aukeratu fitxategia…"));
    auto* browseBtn  = new QPushButton(tr("Browse…"), this);
    pathRow->addWidget(_pathEdit);
    pathRow->addWidget(browseBtn);
    form->addRow(tr("Fitxategia:"), pathRow);

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

    // ── Buttons ─────────────────────────────────────────────
    _buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(browseBtn, &QPushButton::clicked, this, &PuzExportDialog::onBrowse);

    // ── Main layout ─────────────────────────────────────────
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(_buttons);
}

void PuzExportDialog::onBrowse() {
    QString path = QFileDialog::getSaveFileName(
        this, tr("Esportatu .puz gisa"), _pathEdit->text(),
        tr("Across Lite (*.puz)"));
    if (!path.isEmpty())
        _pathEdit->setText(path);
}

QString PuzExportDialog::filePath()  const { return _pathEdit->text().trimmed(); }
QString PuzExportDialog::title()     const { return _titleEdit->text().trimmed(); }
QString PuzExportDialog::author()    const { return _authorEdit->text().trimmed(); }
QString PuzExportDialog::copyright() const { return _copyrightEdit->text().trimmed(); }
