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
#include <QApplication>
#include <QFileOpenEvent>

//----------------------------------------------------------------------------------------------------------------------
//  Application: DECLARATION
//----------------------------------------------------------------------------------------------------------------------
class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);
    ~Application();

#ifdef Q_OS_MACX
    void processFiles();

private:
    QStringList files;
    bool firstRun;
    bool event(QEvent *event);
#endif

};
