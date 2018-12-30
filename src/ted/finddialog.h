/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include "std.h"

namespace Ui{
class FindDialog;
}

class FindDialog : public QDialog{
    Q_OBJECT

public:
    FindDialog( QWidget *parent=nullptr );
    ~FindDialog();

    void readSettings();
    void writeSettings();

    int exec(QString findtext);

    QString findText();
    QString replaceText();
    bool caseSensitive();

signals:

    void findReplace( int how );
    
public slots:

    void onFindNext();
    void onReplace();
    void onReplaceAll();

private:
    Ui::FindDialog *_ui;
    bool _used;
};

#endif // FINDDIALOG_H
