
#include "mainwindow.h"
#include "ui_mainwindow.h"

int main( int argc,char *argv[] ){

#ifdef Q_OS_MACX
    if( QSysInfo::MacintoshVersion>QSysInfo::MV_10_8 ){
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution( ".Lucida Grande UI","Lucida Grande" );
    }
#endif

    QApplication app( argc,argv );

#ifdef Q_OS_MACX
     QDir::setCurrent( QCoreApplication::applicationDirPath()+"/../../.." );
#endif

    MainWindow window;

    window.show();

    window.updateHelp();

    return app.exec();
}
