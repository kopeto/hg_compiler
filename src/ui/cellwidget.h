#pragma once

#include <QWidget>

// ──────────────────────────────────────────────────────────────
// Custom widget that displays a single crossword cell.
// Black cells are painted solid black; fillable cells show
// an editable letter centred in a white square.
// ──────────────────────────────────────────────────────────────
class CellWidget : public QWidget {
    Q_OBJECT
public:
    explicit CellWidget(bool isBlack, QWidget* parent = nullptr);

    void setLetter(char c);
    char letter()  const { return _letter; }
    bool isBlack() const { return _isBlack; }
    void setBlack(bool black);

    static constexpr int CELL_SIZE = 36;

signals:
    void clicked(CellWidget* cell);        // left-click
    void rightClicked(CellWidget* cell);   // right-click (edit mode: toggle black)

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool _isBlack = false;
    char _letter  = '_';
};
