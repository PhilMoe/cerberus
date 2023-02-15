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
#include <QDialog>

namespace Ui {
    class FindDialog;
}

//----------------------------------------------------------------------------------------------------------------------
//  FindDialog: DECLATARTION
//----------------------------------------------------------------------------------------------------------------------
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    FindDialog(QWidget *parent = nullptr);
    ~FindDialog();

    void readSettings();
    void writeSettings();

    int exec(QString findtext);

    QString findText();
    QString replaceText();
    bool caseSensitive();

signals:
    void findReplace(int how);

public slots:
    void onFindNext();
    void onReplace();
    void onReplaceAll();

private:
    Ui::FindDialog *_ui;
    bool _used;
};
