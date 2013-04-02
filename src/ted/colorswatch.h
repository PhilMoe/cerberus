/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef COLORSWATCH_H
#define COLORSWATCH_H

#include "std.h"

class ColorSwatch : public QLabel{
    Q_OBJECT

public:
    ColorSwatch( QWidget *parent );

    QColor color();

public slots:

    void setColor( const QColor &color );

signals:

    void colorChanged();

protected:

    void mousePressEvent( QMouseEvent * ev );

private:
    QColor _color;
};

#endif // COLORSWATCH_H
