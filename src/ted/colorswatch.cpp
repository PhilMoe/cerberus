/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.

Change Log
--------------------------------------------------------------------------------
2019-01-08 - Dawlane
                Possible fix for Colour Swatches turning black when select colour dialog is canceled.
*/

#include "colorswatch.h"

ColorSwatch::ColorSwatch( QWidget *parent ):QLabel( parent ),_color( 0,0,0 ){

    setAutoFillBackground( true );

    setPalette( QPalette( _color ) );
    this->setStyleSheet("background: "+_color.name()+";");
}

QColor ColorSwatch::color(){
    return _color;
}

void ColorSwatch::setColor( const QColor &color ){
    _color=color;

    setPalette( QPalette( _color ) );
    this->setStyleSheet("background: "+_color.name()+";");

    emit colorChanged();
}

void ColorSwatch::mousePressEvent( QMouseEvent *ev ){
    (void)ev;
    //QColorDialog::setStyleSheet("");
    QString ss = this->styleSheet();
    this->setStyleSheet("");
    QColor color = QColorDialog::getColor( _color,this );
    if( !color.isValid())
    {
        this->setStyleSheet(ss);
        return;
    }
    setColor( color );

}
