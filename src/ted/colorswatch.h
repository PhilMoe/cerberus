//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
// CONTRIBUTORS: See contributors.txt
#pragma once

#include <QColor>
#include <QLabel>
#include <QMouseEvent>
#include <QObject>

class ColorSwatch : public QLabel
{
    Q_OBJECT

public:
    ColorSwatch(QWidget *parent);
    QColor color();

public slots:
    void setColor(const QColor &color);

signals:
    void colorChanged();

protected:
    void mousePressEvent(QMouseEvent *ev);

private:
    QColor _color;
};
