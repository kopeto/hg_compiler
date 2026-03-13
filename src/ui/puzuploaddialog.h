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

    QString serverUrl() const;      // full https://host/external/puzzle URL
    QString serverHostOnly() const; // raw host text as typed by the user
    QString apiKey() const;
    QString title() const;
    QString author() const;
    QString copyright() const;

    // Pre-fill metadata fields (call before exec())
    void prefillMetadata(const QString& title, const QString& author, const QString& copyright);

    // Pre-fill server connection fields (call before exec())
    void prefillConnection(const QString& serverHost, const QString& apiKey);

    // Write the edited clue texts back to the GridWord objects.
    void applyClues() const;

private slots:
    void onClueEdited(int row, int col);

private:
    void buildClueTable(Grid& grid);
    void highlightMissingClues();

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
