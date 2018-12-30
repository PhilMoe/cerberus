#ifndef CERBERUSAPPLICATION_H
#define CERBERUSAPPLICATION_H

#include <QApplication>
#include <QFileOpenEvent>
#include "mainwindow.h"

class CerberusApplication : public QApplication
{
    Q_OBJECT

public:
    CerberusApplication(int &argc, char **argv);
    ~CerberusApplication();
    static MainWindow *mainWindow;
#ifdef Q_OS_MACX
    void processFiles();
private:
    QStringList files;
    bool firstRun;
    bool event(QEvent *event);
#endif
};

#endif // CERBERUSAPPLICATION_H
