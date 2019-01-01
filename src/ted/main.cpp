/*
Change Log
-----------------------------------------------------------------------------------------
2018-07-30 - dawlane
        Heavy changes to allow single instance running of Ted. Lets us open multiple files from Explorer.
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cerberusapplication.h"
#include "cerberusguard.h"

int main( int argc,char *argv[] ){

#ifdef Q_OS_MACX
    if( QSysInfo::MacintoshVersion>QSysInfo::MV_10_8 ){
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution( ".Lucida Grande UI","Lucida Grande" );
    }
    CerberusApplication app( argc, argv );
    QDir::setCurrent( QCoreApplication::applicationDirPath()+"/../../.." );
#else
    // CerberusApplication is a derived class of QApplication
    CerberusApplication app( argc, argv );
#endif
    QString name="com.krautapps.Ted"; // This is used to identify our application in the QLocalServer/QLocalSock CerberusGuard
    // Single instance class. Basically creates a QLocalServer/QLocalSocket for inter-process comunication.
    CerberusGuard guardian;
    if(guardian.hasPrevious(name, CerberusApplication::arguments()))
    {
        return 0;
    }
    guardian.listen(name);

// MainWindow is now part of the CerberusApplication as a static member.
#ifdef Q_OS_MACX
        CerberusApplication::mainWindow = new MainWindow;
        CerberusApplication::mainWindow->show();
        CerberusApplication::mainWindow->updateHelp();
        app.processFiles();
#else
        CerberusApplication::mainWindow = new MainWindow;
        CerberusApplication::mainWindow->show();
        CerberusApplication::mainWindow->updateHelp();
#endif
    return app.exec();
}
