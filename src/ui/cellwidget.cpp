#include "cellwidget.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>

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
    _letter  = black ? '#' : '_';
    setFocusPolicy(black ? Qt::NoFocus : Qt::StrongFocus);
    update();
}

void CellWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (_isBlack) {
        p.fillRect(rect(), Qt::black);
    } else {
        // White fill
        p.fillRect(rect(), Qt::white);
        // Border
        p.setPen(QPen(QColor(80, 80, 80), 1));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // Focus ring
        if (hasFocus()) {
            p.setPen(QPen(QColor(0, 120, 215), 2));
            p.drawRect(rect().adjusted(1, 1, -2, -2));
        }

        // Letter
        if (_letter != '_') {
            QFont f = p.font();
            f.setPixelSize(CELL_SIZE * 14 / 20);
            f.setBold(true);
            p.setFont(f);
            p.setPen(Qt::black);
            p.drawText(rect(), Qt::AlignCenter, QString(QChar(std::toupper(_letter))));
        }
    }
}

void CellWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (!_isBlack) setFocus();
        emit clicked(this);
    }
    QWidget::mousePressEvent(event);
}

void CellWidget::keyPressEvent(QKeyEvent* event)
{
    if (_isBlack) { QWidget::keyPressEvent(event); return; }

    int key = event->key();
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        _letter = static_cast<char>('a' + (key - Qt::Key_A));
        update();
    } else if (key == Qt::Key_Backspace || key == Qt::Key_Delete || key == Qt::Key_Space) {
        _letter = '_';
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}
