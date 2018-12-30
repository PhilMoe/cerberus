/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#ifndef PROCESS_H
#define PROCESS_H

//Ugly stuff! But QProcess is doing something very weird on MacOS...

#include "std.h"

#ifdef Q_OS_WIN32
#include <windows.h>
typedef HANDLE pid_t;
typedef HANDLE fd_t;
#else
#include <unistd.h>
typedef int pid_t;
typedef int fd_t;
#endif

class ProcWaiter;
class LineReader;

class Process : public QObject{
    Q_OBJECT

public:
    Process( QObject *parent=nullptr );
    ~Process();

    bool start( const QString &cmd );
    bool wait();
    bool kill();

    bool isEof( int channel );
    bool isLineAvailable( int channel );
    bool waitLineAvailable( int channel,int millis=10000 );
    QString readLine( int channel );

    bool writeLine( const QString &line );

signals:
    void finished();
    void lineAvailable( int channel );

private slots:
    void onFinished();

private:
    pid_t _pid;
    fd_t _in,_out,_err;
    ProcWaiter *_procwaiter;
    LineReader *_linereaders[2];
    int _state;
};

class ProcWaiter : public QThread{
    Q_OBJECT

public:
    ProcWaiter( pid_t pid );

private:
    pid_t _pid;

    void run();
};

class LineReader : public QThread{
    Q_OBJECT

public:
    LineReader( fd_t fd );

    void kill();
    bool isEof();
    bool isLineAvailable();
    bool waitLineAvailable( int millis );
    QString readLine();

private:
    fd_t _fd;
    bool _eof;
    bool _avail;
    QString _line;
    QVector<char> _buf;

    void run();

    int readChar();
};

#endif // PROCESS_H
