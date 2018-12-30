#include "cerberusapplication.h"


CerberusApplication::CerberusApplication(int &argc, char **argv) : QApplication(argc, argv)
{
#ifdef Q_OS_MACX
    firstRun=true;
#endif
}

CerberusApplication::~CerberusApplication()
{
    delete mainWindow;
}

#ifdef Q_OS_MACX
void CerberusApplication::processFiles()
{
    if(!files.empty()){
        foreach(QString file, files){
            CerberusApplication::mainWindow->openFile(file, true);
        }
    }
    firstRun=false;
}

// Capture any Mac OS X events. Only interested in the FileOpen event here.
bool CerberusApplication::event(QEvent *event)
{
    if(event->type()==QEvent::FileOpen)
    {
        QFileOpenEvent * fileOpenEvent = static_cast<QFileOpenEvent *>(event);
        if(fileOpenEvent)
        {
            if(firstRun){
                if(fileOpenEvent->file() != "") files.append(fileOpenEvent->file());
            } else {
                if(fileOpenEvent->file() != "") mainWindow->openFile(fileOpenEvent->file(), true);
            }
        }
    }
    return QApplication::event(event);
}
#endif
