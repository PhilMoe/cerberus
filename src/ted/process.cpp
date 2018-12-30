/*
Ted, a simple text editor/IDE.

Copyright 2012, Blitz Research Ltd.

See LICENSE.TXT for licensing terms.
*/

#include "process.h"

extern void cdebug( const QString &q );

#ifdef Q_OS_WIN

#define CLOSE CloseHandle

#include <tlhelp32.h>

//kill 'em all!
//
static bool killpg( HANDLE proc ){

    QMap<DWORD,DWORD> procs;
    QMap<DWORD,QString> procExes;

    HANDLE snapshot=CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,0 );

    if( snapshot!=INVALID_HANDLE_VALUE ){

        PROCESSENTRY32 pentry={sizeof(pentry)};

        int more=Process32First( snapshot,&pentry );

        while( more ){
            procs.insert( pentry.th32ProcessID,pentry.th32ParentProcessID );
            procExes.insert( pentry.th32ProcessID,QString::fromStdWString( std::wstring(pentry.szExeFile) ) );
            more=Process32Next( snapshot,&pentry );
        }

        DWORD procid=GetProcessId( proc );

        QSet<DWORD> pset;

        QMapIterator<DWORD,DWORD> it( procs );

        while( it.hasNext() ){
            it.next();

            DWORD pid=it.key();
            DWORD ppid=it.value();

            pset.clear();
            while( ppid && ppid!=procid && procs.contains( ppid ) && !pset.contains( ppid ) ){
                ppid=procs.value( ppid );
                pset.insert( ppid );
            }

            if( ppid==procid ){

                HANDLE child=OpenProcess( PROCESS_TERMINATE,0,pid );
                if( child ){
                    if( TerminateProcess( child,-1 ) ){
                        WaitForSingleObject( child,INFINITE );
                    }else{
                        qDebug()<<"Failed to terminate child process"<<procExes.value(pid);
                    }
                    CloseHandle( child );
                }else{
                    qDebug()<<"Failed to open child process"<<procExes.value(pid);
                }
            }
        }

        CloseHandle( snapshot );
    }else{
        qDebug()<<"Failed to create process snapshot";
    }

    if( !TerminateProcess( proc,-1 ) ){
        qDebug()<<"Failed to terminate process";
        return false;
    }

    return true;
}

#else

#define CLOSE close

#include <signal.h>

#include <sys/wait.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

static char **makeargv( const char *cmd ){
    int n,c;
    char *p;
    static char *args,**argv;

    if( args ) free( args );
    if( argv ) free( argv );
    args=(char*)malloc( strlen(cmd)+1 );
    strcpy( args,cmd );

    n=0;
    p=args;
    while( (c=*p++) ){
        if( c==' ' ){
            continue;
        }else if( c=='\"' ){
            while( *p && *p!='\"' ) ++p;
        }else{
            while( *p && *p!=' ' ) ++p;
        }
        if( *p ) ++p;
        ++n;
    }
    argv=(char**)malloc( (n+1)*sizeof(char*) );
    n=0;
    p=args;
    while( (c=*p++) ){
        if( c==' ' ){
            continue;
        }else if( c=='\"' ){
            argv[n]=p;
            while( *p && *p!='\"' ) ++p;
        }else{
            argv[n]=p-1;
            while( *p && *p!=' ' ) ++p;
        }
        if( *p ) *p++=0;
        ++n;
    }
    argv[n]=0;
    return argv;
}

#endif

#define INIT 0
#define RUNNING 1
#define FINISHED 2
#define DELETING 3

Process::Process( QObject *parent ):QObject( parent ),_state( INIT ){
}

bool Process::start( const QString &cmd ){

    if( _state!=INIT ) return false;

#ifdef Q_OS_WIN

    HANDLE in[2],out[2],err[2];
    SECURITY_ATTRIBUTES sa={sizeof(sa),nullptr,1};
    CreatePipe( &in[0],&in[1],&sa,0 );
    CreatePipe( &out[0],&out[1],&sa,0 );
    CreatePipe( &err[0],&err[1],&sa,0 );

    STARTUPINFOW si={sizeof(si)};
    si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
    si.hStdInput=in[0];
    si.hStdOutput=out[1];
    si.hStdError=err[1];
    si.wShowWindow=SW_HIDE;

    PROCESS_INFORMATION pi={nullptr};

    int res=CreateProcessW( 0,(LPWSTR)cmd.toStdWString().c_str(),0,0,-1,CREATE_NEW_PROCESS_GROUP,0,0,&si,&pi );

    CloseHandle( in[0] );
    CloseHandle( out[1] );
    CloseHandle( err[1] );

    if( !res ){
        CloseHandle( in[1] );
        CloseHandle( out[0] );
        CloseHandle( err[0] );
        return false;
    }

    CloseHandle( pi.hThread );

    _pid=pi.hProcess;
    _in=in[1];
    _out=out[0];
    _err=err[0];

#else

    int in[2],out[2],err[2];

    if( pipe( in ) ){}
    if( pipe( out ) ){}
    if( pipe( err ) ){}

    _pid=vfork();

    if( !_pid ){
        #if __linux
            setsid();
        #else
            setpgid(0,0);
        #endif

        dup2( in[PIPE_READ],STDIN_FILENO );
        dup2( out[PIPE_WRITE],STDOUT_FILENO );
        dup2( err[PIPE_WRITE],STDERR_FILENO );

        close( in[PIPE_READ] );
        close( out[PIPE_WRITE] );
        close( err[PIPE_WRITE] );

        close( in[PIPE_WRITE] );
        close( out[PIPE_READ] );
        close( err[PIPE_READ] );

        char **argv=makeargv( cmd.toStdString().c_str() );
        execvp( argv[0],argv );

        _exit( -1 );
        return false;
    }

    close( in[PIPE_READ] );
    close( out[PIPE_WRITE] );
    close( err[PIPE_WRITE] );

    if( _pid==-1 ){
        close( in[PIPE_WRITE] );
        close( out[PIPE_READ] );
        close( err[PIPE_READ] );
        return false;
    }

    _in=in[PIPE_WRITE];
    _out=out[PIPE_READ];
    _err=err[PIPE_READ];

#endif

    _procwaiter=new ProcWaiter( _pid );
    connect( _procwaiter,SIGNAL(finished()),SLOT(onFinished()) );

    _linereaders[0]=new LineReader( _out );
    connect( _linereaders[0],SIGNAL(finished()),SLOT(onFinished()) );

    _linereaders[1]=new LineReader( _err );
    connect( _linereaders[1],SIGNAL(finished()),SLOT(onFinished()) );

    _procwaiter->start();
    _linereaders[0]->start();
    _linereaders[1]->start();

    _state=RUNNING;

    return true;
}

Process::~Process(){

    if( _state!=RUNNING && _state!=FINISHED ) return;

    _state=DELETING;

    if( _procwaiter->wait( 10000 ) ) delete _procwaiter; else cdebug( "Timeout waiting for process to finish" );

    if( _linereaders[0]->wait( 1000 ) ) delete _linereaders[0]; else cdebug( "Timeout waiting for stdout reader to finish" );
    if( _linereaders[1]->wait( 1000 ) ) delete _linereaders[1]; else cdebug( "Timeout waiting for stderr reader to finish" );

    CLOSE( _in );
    CLOSE( _out );
    CLOSE( _err );
}

bool Process::wait(){
    if( _state!=RUNNING ) return _state==FINISHED;
    return _procwaiter->wait( 30000 );
}

bool Process::kill(){
    qDebug()<<"Killing proc";

    if( _state!=RUNNING ) return _state==FINISHED;

    _linereaders[0]->kill();
    _linereaders[1]->kill();

#ifdef Q_OS_WIN
    if( killpg( _pid ) ) return true;
#else
    if( killpg( _pid,SIGTERM )>=0 ) return true;
#endif

    qDebug()<<"proc killed";

    return false;
}

bool Process::writeLine( const QString &line ){
    QString buf=line+'\n';
#ifdef Q_OS_WIN
    DWORD n=0;
    return WriteFile( _in,buf.toStdString().c_str(),buf.length(),&n,nullptr ) && n==buf.length();
#else
    return write( _in,buf.toStdString().c_str(),buf.length() )==buf.length();
#endif
}

bool Process::isEof( int channel ){
    return _linereaders[channel]->isEof();
}

bool Process::isLineAvailable( int channel ){
    return _linereaders[channel]->isLineAvailable();
}

bool Process::waitLineAvailable( int channel,int millis ){
    return _linereaders[channel]->waitLineAvailable( millis );
}

QString Process::readLine( int channel ){
    return _linereaders[channel]->readLine();
}

void Process::onFinished(){

    if( _state!=RUNNING ) return;

    if( sender()==_procwaiter ){
        _state=FINISHED;
        emit finished();
        return;
    }

    for( int channel=0;channel<2;++channel ){
        if( sender()==_linereaders[channel] ){
            if( _linereaders[channel]->isLineAvailable() ){
                emit lineAvailable( channel );
            }
            return;
        }
    }
}

// ***** Proc waiter *****
ProcWaiter::ProcWaiter( pid_t pid ):_pid(pid){
}

void ProcWaiter::run(){
#ifdef Q_OS_WIN
    WaitForSingleObject( _pid,INFINITE );
    CloseHandle( _pid );
#else
    int status=0;
    waitpid( _pid,&status,0 );
#endif
}

// ***** Line reader *****
LineReader::LineReader( fd_t fd ):_fd(fd),_eof(false),_avail(false){
}

void LineReader::kill(){
    _eof=true;
    terminate();
}

bool LineReader::isEof(){
    return _eof;
}

bool LineReader::isLineAvailable(){
    return _avail;
}

bool LineReader::waitLineAvailable( int millis ){
    if( !_avail && !_eof ){
        if( !wait( millis ) ) cdebug( "Timeout waiting for process output" );
    }
    return _avail;
}

QString LineReader::readLine(){
    if( !waitLineAvailable( 10000 ) ) return "";
    QString line=_line;
    _avail=false;
    if( !_eof ) start();
    return line;
}

int LineReader::readChar(){
    unsigned char c;
#ifdef Q_OS_WIN
    DWORD n=0;
    if( ReadFile( _fd,&c,1,&n,nullptr ) && n==1 ) return c;
#else
    if( read( _fd,&c,1 )==1 ) return c;
#endif
    return -1;
}

void LineReader::run(){
    _buf.clear();
    for(;;){
        int c=readChar();
        if( c==-1 ){
            if( _buf.count() ){
                _buf.push_back( 0 );
                _line=QString( _buf.constData() );
                _avail=true;
            }else{
                _line="";
            }
            _eof=true;
            return;
        }else if( c=='\n' ){
            _buf.push_back( 0 );
            _line=QString( _buf.constData() );
            _avail=true;
            return;
        }else if( c!='\r' ){
            _buf.push_back( c );
        }
    }
}
