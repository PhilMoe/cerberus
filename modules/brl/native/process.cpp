
// ***** process.h *****

class BBProcess : public Object{
public:

	BBProcess();
	~BBProcess();

	virtual void Discard();
		
	virtual bool Start( String cmd );
	virtual void Kill( int retCode );
	virtual int  Wait();
	virtual bool IsRunning();
	
	virtual int  StdoutAvail();
	virtual int  ReadStdout( BBDataBuffer *buf,int offset,int count );
	
	virtual int  StderrAvail();
	virtual int  ReadStderr( BBDataBuffer *buf,int offset,int count );
	
	virtual int  WriteStdin( BBDataBuffer *buf,int offset,int count ); 
	
	static String AppPath();
	static Array<String> AppArgs();
	
	static String GetEnv( String key );
	static int SetEnv( String key,String value );
	
	static void Sleep( int millis );
	
	static void ExitApp( int exitCode );
	
	static int ChangeDir( String dir );
	static String CurrentDir();
	
	static int System( String cmd );
	
private:
#if _WIN32
	HANDLE _proc;
	HANDLE _in;
	HANDLE _out;
	HANDLE _err;
	long _exit;
#else
	int _proc;
	int _in;
	int _out;
	int _err;
	int _exit;
#endif
};

// ***** process.cpp *****

#if _WIN32

#else

#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/wait.h>

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

BBProcess::BBProcess():_proc(0),_out(0),_err(0),_in(0),_exit(-1){
}

BBProcess::~BBProcess(){
	Discard();
}

void BBProcess::Discard(){
#if _WIN32
	if( _in ) CloseHandle( _in );
	if( _out ) CloseHandle( _out );
	if( _err ) CloseHandle( _err );
	if( _proc ) CloseHandle( _proc );
#else
	if( _in ) close( _in );
	if( _out ) close( _out );
	if( _err ) close( _err );
	if( _proc ) waitpid( _proc,&_exit,WNOHANG );	//Unix sux here - this will leak zombies if proc isn't done?
#endif
	_in=0;
	_out=0;
	_err=0;
	_proc=0;
	_exit=-1;
}

bool BBProcess::Start( String cmd ){
#if _WIN32

	if( _proc ) return false;
	
    HANDLE in[2],out[2],err[2];
    SECURITY_ATTRIBUTES sa={sizeof(sa),0,1};
    CreatePipe( &in[0],&in[1],&sa,0 );
    CreatePipe( &out[0],&out[1],&sa,0 );
    CreatePipe( &err[0],&err[1],&sa,0 );

    STARTUPINFOW si={sizeof(si)};
    si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
    si.hStdInput=in[0];
    si.hStdOutput=out[1];
    si.hStdError=err[1];
    si.wShowWindow=SW_HIDE;

    PROCESS_INFORMATION pi={0};
    
    DWORD flags=CREATE_NEW_PROCESS_GROUP;
    
    int res=CreateProcessW( 0,(LPWSTR)(const wchar_t*)cmd.ToCString<wchar_t>(),0,0,-1,flags,0,0,&si,&pi );

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

    _in=in[1];
    _out=out[0];
    _err=err[0];
    _proc=pi.hProcess;
    
    return true;

#else

	if( _proc ) return false;
	
    int in[2],out[2],err[2];

    pipe( in );
    pipe( out );
    pipe( err );

    _proc=vfork();

    if( !_proc ){
        #if __linux
            setsid();
        #else
            setpgid(0,0);
        #endif

        dup2( in[0],0 );
        dup2( out[1],1 );
        dup2( err[1],2 );

        close( in[0] );
        close( out[1] );
        close( err[1] );

        close( in[1] );
        close( out[0] );
        close( err[0] );

        char **argv=makeargv( cmd.ToCString<char>() );
        execvp( argv[0],argv );

        ::_exit( -1 );
        return false;
    }

    close( in[0] );
    close( out[1] );
    close( err[1] );

    if( _proc==-1 ){
        close( in[1] );
        close( out[0] );
        close( err[0] );
        return false;
    }

    _in=in[1];
    _out=out[0];
    _err=err[0];
    
    return true;
    
#endif
}

void BBProcess::Kill( int retCode ){
#if _WIN32
	if( !_proc ) return;
	if( TerminateProcess( _proc,retCode ) ){
		WaitForSingleObjectEx( _proc,INFINITE,0 );
	}
	CloseHandle( _proc );
	_exit=retCode;
	_proc=0;
#else
	if( !_proc ) return;
	if( !killpg( _proc,SIGTERM ) ){
		waitpid( _proc,&_exit,0 );
	}
	_exit=retCode;
	_proc=0;
#endif
}

int BBProcess::Wait(){
#if _WIN32
	if( !_proc ) return _exit;
	if( WaitForSingleObjectEx( _proc,INFINITE,0 )==WAIT_OBJECT_0 ){
		GetExitCodeProcess( _proc,(DWORD*)&_exit );
	}
	CloseHandle( _proc );
	_proc=0;
#else
	if( !_proc ) return _exit;
	waitpid( _proc,&_exit,0 );
	_proc=0;
#endif
	return _exit;
}

bool BBProcess::IsRunning(){
#if _WIN32
	if( !_proc ) return false;
	if( WaitForSingleObjectEx( _proc,0,FALSE )!=WAIT_OBJECT_0 ) return true;
	GetExitCodeProcess( _proc,(DWORD*)&_exit );
	CloseHandle( _proc );
	_proc=0;
#else
	if( !_proc ) return false;
	if( !waitpid( _proc,&_exit,WNOHANG ) ) return true;
	_proc=0;
#endif
	return false;
}

int BBProcess::StdoutAvail(){
#if _WIN32
	if( !_out ) return 0;
	DWORD avail=0;
	if( PeekNamedPipe( _out,0,0,0,&avail,0 ) ) return avail;
#else
	int avail=0;
	if( ioctl( _out,FIONREAD,&avail )>=0 ) return avail;
#endif
	return 0;
}

int BBProcess::ReadStdout( BBDataBuffer *buf,int offset,int count ){
#if _WIN32
	if( !_out ) return 0;
	DWORD rcount=0;
	if( ReadFile( _out,buf->WritePointer( offset ),count,&rcount,0 ) ) return rcount;
#else
	int n=read( _out,buf->WritePointer( offset ),count );
	if( n>=0 ) return n;
#endif
	return 0;
}

int BBProcess::StderrAvail(){
#if _WIN32
	if( !_err ) return 0;
	DWORD avail=0;
	if( PeekNamedPipe( _err,0,0,0,&avail,0 ) ) return avail;
#else
	int avail=0;
	if( ioctl( _err,FIONREAD,&avail )>=0 ) return avail;
#endif
	return 0;
}

int BBProcess::ReadStderr( BBDataBuffer *buf,int offset,int count ){
#if _WIN32
	if( !_err ) return 0;
	DWORD rcount=0;
	if( ReadFile( _err,buf->WritePointer( offset ),count,&rcount,0 ) ) return rcount;
#else
	int n=read( _err,buf->WritePointer( offset ),count );
	if( n>=0 ) return n;
#endif
	return 0;
}

int BBProcess::WriteStdin( BBDataBuffer *buf,int offset,int count ){
#if _WIN32
	if( !_in ) return 0;
	DWORD wcount=0;
	if( WriteFile( _in,buf->ReadPointer( offset ),count,&wcount,0 ) ) return wcount;
#else
	int n=write( _in,buf->WritePointer( offset ),count );
	if( n>=0 ) return n;
#endif
	return 0;
}

String BBProcess::AppPath(){

	static String _appPath;
	
	if( _appPath.Length() ) return _appPath;
	
#if _WIN32

	WCHAR buf[MAX_PATH+1];
	GetModuleFileNameW( GetModuleHandleW(0),buf,MAX_PATH );
	buf[MAX_PATH]=0;
	_appPath=String( buf );
	
#elif __APPLE__

	char buf[PATH_MAX];
	uint32_t size=sizeof( buf );
	_NSGetExecutablePath( buf,&size );
	buf[PATH_MAX-1]=0;
	_appPath=String( buf );
	
#elif __linux

	char lnk[PATH_MAX],buf[PATH_MAX];
	pid_t pid=getpid();
	sprintf( lnk,"/proc/%i/exe",pid );
	int i=readlink( lnk,buf,PATH_MAX );
	if( i>0 && i<PATH_MAX ){
		buf[i]=0;
		_appPath=String( buf );
	}

#endif

//	_appPath=RealPath( _appPath );
	return _appPath;

}

Array<String> BBProcess::AppArgs(){

	static Array<String> _appArgs;
	
	if( _appArgs.Length() ) return _appArgs;
	
	_appArgs=Array<String>( argc );
	for( int i=0;i<argc;++i ){
		_appArgs[i]=String( argv[i] );
	}
	return _appArgs;
}

String BBProcess::GetEnv( String key ){
#if _WIN32
	if( WCHAR *p=_wgetenv( key.ToCString<WCHAR>() ) ) return String( p );
#else
	if( char *p=getenv( key.ToCString<char>() ) ) return String( p );
#endif
	return "";
}

int BBProcess::SetEnv( String key,String value ){
#if _WIN32
	return _wputenv( ( key+"="+value ).ToCString<WCHAR>() );
#else
	if( value.Length() ) return setenv( key.ToCString<char>(),value.ToCString<char>(),1 );
	unsetenv( key.ToCString<char>() );
	return 0;
#endif
}

void BBProcess::Sleep( int millis ){
#if _WIN32
	::Sleep( millis );
#else
	usleep( millis*1000 );
#endif
}

void BBProcess::ExitApp( int exitCode ){
	exit( exitCode );
}

int BBProcess::ChangeDir( String path ){
#if _WIN32
	if( !SetCurrentDirectoryW( path.ToCString<WCHAR>() ) ) return -1;
	return 0;
#else
	return chdir( path.ToUtf8() );
#endif
}

String BBProcess::CurrentDir(){
#if _WIN32
	WCHAR buf[MAX_PATH];
	if( !GetCurrentDirectoryW( MAX_PATH,buf ) ) return -1;
	buf[MAX_PATH-1]=0;
	return String( buf ).Replace( "\\","/" );
#else
	char buf[PATH_MAX];
	if( !getcwd( buf,PATH_MAX ) ) return -1;
	buf[PATH_MAX-1]=0;
	return buf;
#endif
}

int BBProcess::System( String cmd ){

#if _WIN32

	cmd=String("cmd /S /C \"")+cmd+"\"";

	PROCESS_INFORMATION pi={0};
	STARTUPINFOW si={sizeof(si)};
	
	if( !CreateProcessW( 0,(LPWSTR)(const WCHAR*)cmd.ToCString<WCHAR>(),0,0,1,CREATE_DEFAULT_ERROR_MODE,0,0,&si,&pi ) ) return -1;

	WaitForSingleObject( pi.hProcess,INFINITE );
	
	int res=GetExitCodeProcess( pi.hProcess,(DWORD*)&res ) ? res : -1;

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return res;

#else

	return system( cmd.ToCString<char>() );

#endif

}
