/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "colorswatch.h"

ColorSwatch::ColorSwatch( QWidget *parent ):QLabel( parent ),_color( 0,0,0 ){

    setAutoFillBackground( true );

    setPalette( QPalette( _color ) );
}

QColor ColorSwatch::color(){
    return _color;
}

void ColorSwatch::setColor( const QColor &color ){
    _color=color;

    setPalette( QPalette( _color ) );

    emit colorChanged();
}

void ColorSwatch::mousePressEvent( QMouseEvent *ev ){
    setColor( QColorDialog::getColor( _color,this ) );
}
