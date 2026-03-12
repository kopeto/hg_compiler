#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>

// ──────────────────────────────────────────────────────────────
// Dialog shown before exporting a .puz file.
// Lets the user choose the output path and fill in metadata:
//   Izenburua (Title), Egilea (Author), Copyright.
// ──────────────────────────────────────────────────────────────
class PuzExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit PuzExportDialog(QWidget* parent = nullptr);

    QString filePath()  const;
    QString title()     const;
    QString author()    const;
    QString copyright() const;

private slots:
    void onBrowse();

private:
    QLineEdit*        _pathEdit      = nullptr;
    QLineEdit*        _titleEdit     = nullptr;
    QLineEdit*        _authorEdit    = nullptr;
    QLineEdit*        _copyrightEdit = nullptr;
    QDialogButtonBox* _buttons       = nullptr;
};
