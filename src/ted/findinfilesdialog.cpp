/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "findinfilesdialog.h"
#include "ui_findinfilesdialog.h"

#include "prefs.h"

FindInFilesDialog::FindInFilesDialog( QWidget *parent ):QDialog( parent ),_ui( new Ui::FindInFilesDialog ),_used( false ){

    _ui->setupUi( this );
}

FindInFilesDialog::~FindInFilesDialog(){

    delete _ui;
}

void FindInFilesDialog::readSettings(){

    QSettings settings;

    if( settings.value( "settingsVersion" ).toInt()<2 ){
        _ui->typesLineEdit->setText( "*.cxs" );
        _ui->dirLineEdit->setText( Prefs::prefs()->getString( "cerberusPath" ) );
        return;
    }

    settings.beginGroup( "findInFilesDialog" );

    _ui->findLineEdit->setText( settings.value( "findText" ).toString() );
    _ui->typesLineEdit->setText( settings.value( "fileTypes" ).toString() );
    _ui->dirLineEdit->setText( fixPath( settings.value( "directory" ).toString() ) );
    _ui->casedCheckBox->setChecked( settings.value( "caseSensitive" ).toBool() );
    _ui->recursiveCheckBox->setChecked( settings.value( "recursive" ).toBool() );

    restoreGeometry( settings.value( "geometry" ).toByteArray() );

    settings.endGroup();
}

void FindInFilesDialog::writeSettings(){

    QSettings settings;

    settings.beginGroup( "findInFilesDialog" );

    settings.setValue( "findText",_ui->findLineEdit->text() );
    settings.setValue( "fileTypes",_ui->typesLineEdit->text() );
    settings.setValue( "directory",_ui->dirLineEdit->text() );
    settings.setValue( "caseSensitive",_ui->casedCheckBox->isChecked() );
    settings.setValue( "recursive",_ui->recursiveCheckBox->isChecked() );

    if( _used ) settings.setValue( "geometry",saveGeometry() );

    settings.endGroup();
}

void FindInFilesDialog::show(){
    QDialog::show();

    if( !_used ){
        restoreGeometry( saveGeometry() );
        _used=true;
    }
}

void FindInFilesDialog::show( const QString &path ){
    _ui->dirLineEdit->setText( path );
    show();
}

void FindInFilesDialog::find(){

    QString findText=_ui->findLineEdit->text();
    if( findText.isEmpty() ) return;

    QString dir=_ui->dirLineEdit->text();
    if( dir.isEmpty() ) return;

    bool cased=_ui->casedCheckBox->isChecked();

    bool recursive=_ui->recursiveCheckBox->isChecked();

    _pos.clear();
    _ui->resultsListWidget->clear();
    _len=findText.length();

    QString tfilters=_ui->typesLineEdit->text();
    tfilters=tfilters.replace( ","," " );
    tfilters=tfilters.replace( ";"," " );
    tfilters=tfilters.replace( "|"," " );
    QStringList filters=tfilters.split( " ",QString::SkipEmptyParts );

    QStack<QString> todo;
    todo.push( dir );

    QDir::Filters eflags=QDir::Files;
    if( recursive ) eflags|=QDir::AllDirs;

    Qt::CaseSensitivity cflags=cased ? Qt::CaseSensitive : Qt::CaseInsensitive;

    _cancel=false;
    _ui->findButton->setEnabled( false );
    _ui->findLineEdit->setEnabled( false );
    _ui->typesLineEdit->setEnabled( false );
    _ui->dirLineEdit->setEnabled( false );
    _ui->casedCheckBox->setEnabled( false );
    _ui->recursiveCheckBox->setEnabled( false );
    _ui->cancelButton->setEnabled( true );

    while( !todo.isEmpty() ){

        QString dir=todo.pop();

        QStringList files=QDir( dir ).entryList( filters,eflags );

        for( int i=0;i<files.size();++i ){

            if( files.at( i )=="." || files.at( i )==".." ) continue;

            QString path=dir+"/"+files.at( i );

            if( QFileInfo( path ).isDir() ){
                todo.push( path );
                continue;
            }

            //Ok, search the file!
            //
            QFile file( path );
            if( !file.open( QIODevice::ReadOnly ) ){
                qDebug()<<"Find in files failed to open file "<<path;
                continue;
            }
            QTextStream stream( &file );
            QString text=stream.readAll();
            file.close();

            int j=0,k=0,nl=0,cr=0;
            for(;;){
                j=text.indexOf( findText,j,cflags );
                if( j==-1 ) break;

                for( ;k<j;++k ){
                    if( text[k]=='\n' ) ++nl; else if( text[k]=='\r' ) ++cr;
                }

                _pos.append( j-cr );
                _ui->resultsListWidget->addItem( path+"<"+QString::number(nl+1)+">" );

                j+=findText.length();
            }

            QCoreApplication::processEvents();

            if( _cancel ) break;

        }
        if( _cancel ) break;
    }
    _ui->findLineEdit->setEnabled( true );
    _ui->typesLineEdit->setEnabled( true );
    _ui->dirLineEdit->setEnabled( true );
    _ui->casedCheckBox->setEnabled( true );
    _ui->recursiveCheckBox->setEnabled( true );
    _ui->cancelButton->setEnabled( false );
    _ui->findButton->setEnabled( true );
}

void FindInFilesDialog::cancel(){
    _cancel=true;
}

void FindInFilesDialog::browseForDir(){

    QString dir=_ui->dirLineEdit->text();

    QString path=fixPath( QFileDialog::getExistingDirectory( this,"Select directory",dir,QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks ) );
    if( path.isEmpty() ) return;

    _ui->dirLineEdit->setText( path );
}

void FindInFilesDialog::showResult( QListWidgetItem *item ){

    int pos=-1;
    for( int i=0;i<_ui->resultsListWidget->count();++i ){
        if( _ui->resultsListWidget->item( i )==item ){
            pos=_pos.at( i );
            break;
        }
    }
    if( pos==-1 ) return;

    QString info=item->text();
    int i=info.lastIndexOf( '<' );
    if( i!=-1 && info.endsWith( '>' ) ){
        QString path=info.left( i );
//        int line=info.mid( i+1,info.length()-i-2 ).toInt()-1;
//        emit showCode( path,line );
        emit showCode( path,pos,_len );
    }
}
