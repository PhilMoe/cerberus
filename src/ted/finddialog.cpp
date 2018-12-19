/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog( QWidget *parent ):QDialog( parent ),_ui( new Ui::FindDialog ),_used( false ){

    _ui->setupUi( this );
}

FindDialog::~FindDialog(){

    delete _ui;
}

void FindDialog::readSettings(){

    QSettings settings;

    if( settings.value( "settingsVersion" ).toInt()<2 ) return;

    settings.beginGroup( "findDialog" );

    _ui->findText->setText( settings.value( "findText" ).toString() );
    _ui->replaceText->setText( settings.value( "replaceText" ).toString() );
    _ui->caseSensitive->setChecked( settings.value( "caseSensitive" ).toBool() );

    restoreGeometry( settings.value( "geometry" ).toByteArray() );

    settings.endGroup();
}

void FindDialog::writeSettings(){
    if( !_used ) return;

    QSettings settings;

    settings.beginGroup( "findDialog" );

    settings.setValue( "findText",_ui->findText->text() );
    settings.setValue( "replaceText",_ui->replaceText->text() );
    settings.setValue( "caseSensitive",_ui->caseSensitive->isChecked() );

    settings.setValue( "geometry",saveGeometry() );

    settings.endGroup();
}

int FindDialog::exec(QString findText){
    QDialog::show();

    if( !_used ){
        restoreGeometry( saveGeometry() );
        _used=true;
    }
    if (findText.length()>0)
        _ui->findText->setText(findText);
    _ui->findText->setFocus( Qt::OtherFocusReason );
    _ui->findText->selectAll();

    return QDialog::exec();
}

QString FindDialog::findText(){
    return _ui->findText->text();
}

QString FindDialog::replaceText(){
    return _ui->replaceText->text();
}

bool FindDialog::caseSensitive(){
    return _ui->caseSensitive->isChecked();
}

void FindDialog::onFindNext(){
    emit findReplace( 0 );
}

void FindDialog::onReplace(){
    emit findReplace( 1 );
}

void FindDialog::onReplaceAll(){
    emit findReplace( 2 );
}


