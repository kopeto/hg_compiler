#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTableWidget>
#include <vector>

struct Grid;
struct GridWord;

// ──────────────────────────────────────────────────────────────
// Dialog for uploading a .puz to a remote server via HTTP POST.
// Similar to PuzExportDialog but replaces the file-path browse
// with server-URL and API-key fields.
// ──────────────────────────────────────────────────────────────
class PuzUploadDialog : public QDialog {
    Q_OBJECT
public:
    explicit PuzUploadDialog(Grid& grid, QWidget* parent = nullptr);

    QString serverUrl() const;
    QString apiKey() const;
    QString title() const;
    QString author() const;
    QString copyright() const;

    // Write the edited clue texts back to the GridWord objects.
    void applyClues() const;

private:
    void buildClueTable(Grid& grid);

    QLineEdit*        _serverEdit    = nullptr;
    QLineEdit*        _apiKeyEdit    = nullptr;
    QLineEdit*        _titleEdit     = nullptr;
    QLineEdit*        _authorEdit    = nullptr;
    QLineEdit*        _copyrightEdit = nullptr;
    QTableWidget*     _clueTable     = nullptr;
    QDialogButtonBox* _buttons       = nullptr;

    // One entry per data row (group-header rows store nullptr)
    std::vector<GridWord*> _rowToWord;
};
