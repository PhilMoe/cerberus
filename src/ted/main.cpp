//----------------------------------------------------------------------------------------------------------------------
// Ted, a simple text editor/IDE.
//
// Copyright 2012, Blitz Research Ltd.
//
// See LICENSE.TXT for licensing terms.
//
//  NOTE: This version is not backwards compatible with versions earlier than Qt 5.9.0
//----------------------------------------------------------------------------------------------------------------------
#include "application.h"
#include "applicationguard.h"
#include "mainwindow.h"

#ifdef Q_OS_MACX
    #include <QDir>
#endif

int main(int argc, char *argv[])
{

#ifdef Q_OS_MACX
    Application app(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath() + "/../../..");
#else
    // Application is a derived class of QApplication
    Application app(argc, argv);
#endif
    QString name =
        "com.krautapps.Ted"; // This is used to identify our application in the QLocalServer/QLocalSock CerberusGuard

    // Single instance class. Basically creates a QLocalServer/QLocalSocket for inter-process communication.
    ApplicationGuard guardian;
    if(guardian.hasPrevious(name, Application::arguments()))
        return 0;
    guardian.listen(name);

    // MainWindow is now part of the CerberusApplication as a static member.
    MAIN_WINDOW = new MainWindow;

    MAIN_WINDOW->show();
    MAIN_WINDOW->updateHelp();
#ifdef Q_OS_MACX
    app.processFiles();
#endif
    return app.exec();
}
