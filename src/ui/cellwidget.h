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
    bool isFixed() const { return _fixed; }
    void setBlack(bool black);
    void setFixed(bool fixed);   // lock/unlock the cell

    // Selection highlight: 0 = none, 1 = word highlight, 2 = active cell
    void setHighlight(int level);
    int  highlight() const { return _highlight; }

    static constexpr int CELL_SIZE = 36;

signals:
    void clicked(CellWidget* cell);            // left-click
    void rightClicked(CellWidget* cell);        // right-click
    void fixToggled(CellWidget* cell);          // letter written or erased
    void keyNavigate(CellWidget* cell, int key);// arrow keys / backspace navigation

protected:    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool _isBlack  = false;
    bool _fixed    = false;
    char _letter   = '_';
    int  _highlight = 0;   // 0=none, 1=word, 2=active cell
};
