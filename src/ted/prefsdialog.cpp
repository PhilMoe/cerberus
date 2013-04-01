/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "prefsdialog.h"
#include "ui_prefsdialog.h"

#include "prefs.h"

PrefsDialog::PrefsDialog( QWidget *parent ):QDialog( parent ),_ui( new Ui::PrefsDialog ),_prefs( Prefs::prefs() ),_used( false ){
    _ui->setupUi( this );
}

PrefsDialog::~PrefsDialog(){
    delete _ui;
}

void PrefsDialog::readSettings(){

    QSettings settings;

    if( settings.value( "settingsVersion" ).toInt()<2 ){

        return;
    }

    settings.beginGroup( "prefsDialog" );

    restoreGeometry( settings.value( "geometry" ).toByteArray() );

    settings.endGroup();
}

void PrefsDialog::writeSettings(){
    QSettings settings;

    settings.beginGroup( "prefsDialog" );

    if( _used ) settings.setValue( "geometry",saveGeometry() );

    settings.endGroup();
}

int PrefsDialog::exec(){
    QDialog::show();

    if( !_used ){
        restoreGeometry( saveGeometry() );
        _used=true;
    }

    _ui->fontComboBox->setCurrentFont( QFont( _prefs->getString( "fontFamily" ),_prefs->getInt("fontSize") ) );
    _ui->fontSizeWidget->setValue( _prefs->getInt("fontSize") );
    _ui->tabSizeWidget->setValue( _prefs->getInt( "tabSize" ) );
    _ui->smoothFontsWidget->setChecked( _prefs->getBool( "smoothFonts" ) );

    _ui->backgroundColorWidget->setColor( _prefs->getColor( "backgroundColor" ) );
    _ui->defaultColorWidget->setColor( _prefs->getColor( "defaultColor" ) );
    _ui->numbersColorWidget->setColor( _prefs->getColor( "numbersColor" ) );
    _ui->stringsColorWidget->setColor( _prefs->getColor( "stringsColor" ) );
    _ui->identifiersColorWidget->setColor( _prefs->getColor( "identifiersColor" ) );
    _ui->keywordsColorWidget->setColor( _prefs->getColor( "keywordsColor" ) );
    _ui->commentsColorWidget->setColor( _prefs->getColor( "commentsColor" ) );
    _ui->highlightColorWidget->setColor( _prefs->getColor( "highlightColor" ) );
    _ui->tabSizeWidget->setValue( _prefs->getInt( "tabSize" ) );

    _ui->monkeyPathWidget->setText( _prefs->getString( "monkeyPath" ) );
    _ui->blitzmaxPathWidget->setText( _prefs->getString( "blitzmaxPath" ) );

    return QDialog::exec();
}

void PrefsDialog::onFontChanged( const QFont &font ){
    _prefs->setValue( "fontFamily",font.family() );
}

void PrefsDialog::onFontSizeChanged( int size ){
    _prefs->setValue( "fontSize",size );
}

void PrefsDialog::onTabSizeChanged( int size ){
    _prefs->setValue( "tabSize",size );
}

void PrefsDialog::onSmoothFontsChanged( bool state ){
    _prefs->setValue( "smoothFonts",state );
}

void PrefsDialog::onColorChanged(){

    ColorSwatch *swatch=qobject_cast<ColorSwatch*>( sender() );
    if( !swatch ) return;

    QString name=swatch->objectName();
    int i=name.indexOf( "Widget" );
    if( i==-1 ) return;
    name=name.left( i );

    _prefs->setValue( name.toStdString().c_str(),swatch->color() );
}

void PrefsDialog::onBrowseForPath(){

    if( sender()==_ui->monkeyPathButton ){

        QString path=QFileDialog::getExistingDirectory( this,"Select Monkey directory","",QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks );
        if( path.isEmpty() ) return;
        path=fixPath( path );

        _prefs->setValue( "monkeyPath",path );
        _ui->monkeyPathWidget->setText( path );

    }else if( sender()==_ui->blitzmaxPathButton ){

        QString path=QFileDialog::getExistingDirectory( this,"Select BlitzMax directory","",QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks );
        if( path.isEmpty() ) return;
        path=fixPath( path );

        _prefs->setValue( "blitzmaxPath",path );
        _ui->blitzmaxPathWidget->setText( path );
    }
}
