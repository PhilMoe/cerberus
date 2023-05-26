//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// This basically changes the button colours in the preference dialogue. The colour names match those in the prefsdialog
// ui file.
// CONTRIBUTORS: See contributors.txt
#include "colorswatch.h"
#include <QColorDialog>

ColorSwatch::ColorSwatch(QWidget *parent) : QLabel(parent), _color(0, 0, 0)
{
    setAutoFillBackground(true);

    setPalette(QPalette(_color));
    this->setStyleSheet("background: " + _color.name() + ";");
}

QColor ColorSwatch::color()
{
    return _color;
}

void ColorSwatch::setColor(const QColor &color)
{
    _color = color;

    setPalette(QPalette(_color));
    this->setStyleSheet("background: " + _color.name() + ";");

    emit colorChanged();
}

void ColorSwatch::mousePressEvent(QMouseEvent *ev)
{
    static_cast<void>(ev);
    QString ss = this->styleSheet();
    this->setStyleSheet("");
    QColor color = QColorDialog::getColor(_color, this);
    if(!color.isValid()) {
        this->setStyleSheet(ss);
        return;
    }
    setColor(color);
}
