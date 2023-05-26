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
#include "application.h"
#include "mainwindow.h"
#include <QEvent>
#include <QFileOpenEvent>

//----------------------------------------------------------------------------------------------------------------------
//  Application: IMPLEMENTETION
//----------------------------------------------------------------------------------------------------------------------
// NOTE: Extent the QApplication class. This is required for Mac OS X.
Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
#ifdef Q_OS_MACX
    firstRun = true;
#endif
}

Application::~Application()
{
    delete MAIN_WINDOW;
}

#ifdef Q_OS_MACX
//----------------------------------------------------------------------------------------------------------------------
//  Application: PUBLIC MEMEBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
void Application::processFiles()
{
    if(!files.empty()) {
        foreach(QString file, files)
            MAIN_WINDOW->openFile(file, true);
    }
    firstRun = false;
}

//----------------------------------------------------------------------------------------------------------------------
//  Application: PRIVATE MEMEBER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------
// Capture any Mac OS X events. Only interested in the FileOpen event here.
bool Application::event(QEvent *event)
{
    if(event->type() == QEvent::FileOpen) {
        QFileOpenEvent *fileOpenEvent = static_cast<QFileOpenEvent *>(event);
        if(fileOpenEvent) {
            if(firstRun) {
                if(fileOpenEvent->file() != "")
                    files.append(fileOpenEvent->file());
            } else {
                if(fileOpenEvent->file() != "")
                    MAIN_WINDOW->openFile(fileOpenEvent->file(), true);
            }
        }
    }
    return QApplication::event(event);
}
#endif
