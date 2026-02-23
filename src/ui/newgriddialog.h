#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QVector>

#include "gridwidget.h"

// ──────────────────────────────────────────────────────────────
// Dialog to create a new blank grid.
//
// Shows a live preview GridWidget (edit mode ON) so the user
// can place black cells with right-click before confirming.
// Changing the spinboxes rebuilds the preview.
// ──────────────────────────────────────────────────────────────
class NewGridDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewGridDialog(QWidget* parent = nullptr);

    int gridRows() const;
    int gridCols() const;

    // Returns the black/white layout chosen in the preview.
    // '#' = black cell, '.' = white cell
    QVector<QVector<char>> layout() const;

private slots:
    void rebuildPreview();

private:
    QSpinBox*          _rowsSpin   = nullptr;
    QSpinBox*          _colsSpin   = nullptr;
    GridWidget*        _preview    = nullptr;
    QScrollArea*       _scroll     = nullptr;
    QDialogButtonBox*  _buttons    = nullptr;
};
