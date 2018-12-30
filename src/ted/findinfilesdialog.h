/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef FINDINFILES_H
#define FINDINFILES_H

#include "std.h"

namespace Ui {
class FindInFilesDialog;
}

class FindInFilesDialog : public QDialog{
    Q_OBJECT
    
public:
    FindInFilesDialog( QWidget *parent=nullptr );
    ~FindInFilesDialog();

    void readSettings();
    void writeSettings();

    void show();
    void show( const QString &path );

public slots:

    void find();

    void cancel();

    void browseForDir();

    void showResult( QListWidgetItem *item );

signals:

    void showCode( const QString &path,int pos,int length );

private:
    Ui::FindInFilesDialog *_ui;
    bool _used;

    QList<int> _pos;
    int _len;

    bool _cancel;
};

#endif // FINDINFILES_H
