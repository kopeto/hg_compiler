#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTableWidget>
#include <vector>

struct Grid;
struct GridWord;

// ──────────────────────────────────────────────────────────────
// Dialog shown before exporting a .puz file.
// Shows metadata fields and a table with every word/clue pair.
// Clues are editable; call applyClues() after accept() to
// write them back to the GridWord objects in the app state.
// ──────────────────────────────────────────────────────────────
class PuzExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit PuzExportDialog(Grid& grid, QWidget* parent = nullptr);

    QString filePath() const;
    QString title() const;
    QString author() const;
    QString copyright() const;

    // Write the edited clue texts back to the GridWord objects.
    void applyClues() const;

private slots:
    void onBrowse();

private:
    void buildClueTable(Grid& grid);

    QLineEdit*        _pathEdit      = nullptr;
    QLineEdit*        _titleEdit     = nullptr;
    QLineEdit*        _authorEdit    = nullptr;
    QLineEdit*        _copyrightEdit = nullptr;
    QTableWidget*     _clueTable     = nullptr;
    QDialogButtonBox* _buttons       = nullptr;

    // One entry per data row (group-header rows store nullptr)
    std::vector<GridWord*> _rowToWord;
};
