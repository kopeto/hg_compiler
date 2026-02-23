#include "cellwidget.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <cctype>

CellWidget::CellWidget(bool isBlack, QWidget* parent)
    : QWidget(parent), _isBlack(isBlack)
{
    setFixedSize(CELL_SIZE, CELL_SIZE);
    setFocusPolicy(_isBlack ? Qt::NoFocus : Qt::StrongFocus);
}

void CellWidget::setLetter(char c)
{
    if (_isBlack) return;
    _letter = (c == '.' || c == '\0') ? '_' : c;
    update();
}

void CellWidget::setBlack(bool black)
{
    _isBlack = black;
    _fixed   = false;
    _letter  = black ? '#' : '_';
    setFocusPolicy(black ? Qt::NoFocus : Qt::StrongFocus);
    update();
}

void CellWidget::setFixed(bool fixed)
{
    if (_isBlack) return;
    _fixed = fixed;
    update();
}

void CellWidget::setHighlight(int level)
{
    if (_highlight == level) return;
    _highlight = level;
    update();
}

void CellWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (_isBlack) {
        p.fillRect(rect(), Qt::black);
    } else {
        // Background — priority: fixed > active-cell > word-highlight > plain
        QColor bg = Qt::white;
        if (_fixed)           bg = QColor(210, 228, 255);
        else if (_highlight == 2) bg = QColor(255, 255, 100);   // active cell: yellow
        else if (_highlight == 1) bg = QColor(168, 210, 255);   // word: light blue

        p.fillRect(rect(), bg);

        // Border
        p.setPen(QPen(_fixed ? QColor(60, 120, 220) : QColor(80, 80, 80),
                      _fixed ? 2 : 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // Focus ring (only when not fixed and not highlighted)
        if (hasFocus() && !_fixed && _highlight == 0) {
            p.setPen(QPen(QColor(0, 120, 215), 2));
            p.drawRect(rect().adjusted(1, 1, -2, -2));
        }

        // Letter
        if (_letter != '_') {
            QFont f = p.font();
            f.setPixelSize(CELL_SIZE * 14 / 20);
            f.setBold(true);
            p.setFont(f);
            p.setPen(_fixed ? QColor(30, 90, 200) : Qt::black);
            p.drawText(rect(), Qt::AlignCenter, QString(QChar(std::toupper(_letter))));
        }
    }
}

void CellWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (!_isBlack) setFocus();
        emit clicked(this);
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked(this);
    }
    QWidget::mousePressEvent(event);
}

void CellWidget::keyPressEvent(QKeyEvent* event)
{
    if (_isBlack) { QWidget::keyPressEvent(event); return; }

    int key = event->key();
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        _letter = static_cast<char>('a' + (key - Qt::Key_A));
        _fixed  = true;
        update();
        emit fixToggled(this);
        emit keyNavigate(this, Qt::Key_Tab); // Tab = "advance along current word"
    } else if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
        _letter = '_';
        _fixed  = false;
        update();
        emit fixToggled(this);
        if (key == Qt::Key_Backspace)
            emit keyNavigate(this, Qt::Key_Backspace); // move back
    } else if (key == Qt::Key_Space) {
        _letter = '_';
        _fixed  = false;
        update();
        emit fixToggled(this);
    } else if (key == Qt::Key_Left  || key == Qt::Key_Right ||
               key == Qt::Key_Up    || key == Qt::Key_Down) {
        emit keyNavigate(this, key);
    } else {
        QWidget::keyPressEvent(event);
    }
}
