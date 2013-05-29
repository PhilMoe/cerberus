
// ***** tcpstream.h *****

#if WINDOWS_8

#else

#if _WIN32

#include <winsock.h>

#elif WINDOWS_PHONE_8

#include <Winsock2.h>

#else

#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define closesocket close
#define ioctlsocket ioctl

#endif

class BBTcpStream : public BBStream{
public:

	BBTcpStream();
	BBTcpStream( int sock );
	~BBTcpStream();
	
	bool Connect( String addr,int port );
	int ReadAvail();
	int WriteAvail();
	
	int Eof();
	void Close();
	int Read( BBDataBuffer *buffer,int offset,int count );
	int Write( BBDataBuffer *buffer,int offset,int count );
	
	static void InitSockets();
	
private:
	int _sock;
	int _state;	//0=INIT, 1=CONNECTED, 2=CLOSED, -1=ERROR
	
	void SetSockOpts();
};

// ***** tcpstream.cpp *****

void BBTcpStream::InitSockets(){
#if _WIN32
	static bool started;
	if( !started ){
		WSADATA ws;
		WSAStartup( 0x101,&ws );
		started=true;
	}
#endif
}

BBTcpStream::BBTcpStream():_sock( -1 ),_state( 0 ){
	InitSockets();
}

BBTcpStream::BBTcpStream( int sock ):_sock( sock ),_state( 1 ){
	int nodelay=1;
	setsockopt( _sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nodelay,sizeof(nodelay) );
}

BBTcpStream::~BBTcpStream(){
	if( _sock>=0 ) closesocket( _sock );
}

bool BBTcpStream::Connect( String addr,int port ){
	if( _state ) return false;

	if( addr.Length()>1023 ) return false;
	
	char buf[1024];
	for( int i=0;i<addr.Length();++i ) buf[i]=addr[i];
	buf[addr.Length()]=0;

	_sock=socket( AF_INET,SOCK_STREAM,IPPROTO_TCP );
	if( _sock>=0 ){
		if( hostent *host=gethostbyname( buf ) ){
			if( char *hostip=inet_ntoa(*(struct in_addr *)*host->h_addr_list) ){
				struct sockaddr_in sa;
				sa.sin_family=AF_INET;
				sa.sin_addr.s_addr=inet_addr( hostip );
				sa.sin_port=htons( port );
				if( connect( _sock,(const sockaddr*)&sa,sizeof(sa) )>=0 ){
					int nodelay=1;
					setsockopt( _sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nodelay,sizeof(nodelay) );
					_state=1;
					return true;
				}
			}
		}
		closesocket( _sock );
		_sock=-1;
	}
	return false;
}

int BBTcpStream::ReadAvail(){

	if( _state!=1 ) return 0;
	
#ifdef FIONREAD	
	u_long arg;
	if( ioctlsocket( _sock,FIONREAD,&arg )>=0 ) return arg;
	_state=-1;
#endif

	return 0;
}

int BBTcpStream::WriteAvail(){

	if( _state!=1 ) return 0;

#ifdef FIONWRITE
	u_long arg;
	if( ioctlsocket( _sock,FIONREAD,&arg )>=0 ) return arg;
	_state=-1;
#endif

	return 0;
}

int BBTcpStream::Eof(){
	if( _state>=0 ) return _state==2;
	return -1;
}

void BBTcpStream::Close(){
	if( _sock<0 ) return;
	if( _state==1 ) _state=2;
	closesocket( _sock );
	_sock=-1;
}

int BBTcpStream::Read( BBDataBuffer *buffer,int offset,int count ){
	if( _state!=1 ) return 0;
	int n=recv( _sock,(char*)buffer->WritePointer(offset),count,0 );
	if( n>0 || (n==0 && count==0) ) return n;
	_state=(n==0) ? 2 : -1;
	return 0;
	
}

int BBTcpStream::Write( BBDataBuffer *buffer,int offset,int count ){
	if( _state!=1 ) return 0;
	int n=send( _sock,(const char*)buffer->ReadPointer(offset),count,0 );
	if( n>=0 ) return n;
	_state=-1;
	return 0;
}

#endif

#if WINDOWS_8

// ***** Windows Store app are a PITA *****

class BBTcpStream : public BBStream{
public:

	BBTcpStream();
	~BBTcpStream();
	
	bool Connect( String addr,int port );
	int ReadAvail();
	int WriteAvail();
	
	int Eof();
	void Close();
	int Read( BBDataBuffer *buffer,int offset,int count );
	int Write( BBDataBuffer *buffer,int offset,int count );

private:

	friend struct ConnectHandler;
	friend struct LoadHandler;
	friend struct StoreHandler;
	
	struct ConnectHandler{
		BBTcpStream *_stream;
		ConnectHandler( BBTcpStream *stream ):_stream( stream ){}
		void operator()( IAsyncAction ^info,AsyncStatus status ){
			_stream->OnConnect( info,status );
		}
	};
	
	struct LoadHandler{
		BBTcpStream *_stream;
		LoadHandler( BBTcpStream *stream ):_stream( stream ){}
		void operator()( IAsyncOperation<unsigned int> ^info,AsyncStatus status ){
			_stream->OnLoad( info,status );
		}
	};
	
	struct StoreHandler{
		BBTcpStream *_stream;
		StoreHandler( BBTcpStream *stream ):_stream( stream ){}
		void operator()( IAsyncOperation<unsigned int> ^info,AsyncStatus status ){
			_stream->OnStore( info,status );
		}
	};

	//0=INIT, 1=CONNECTED, 2=CLOSED, -1=ERROR
	int _state;	
	HANDLE _revent,_wevent;
	
	Windows::Networking::Sockets::StreamSocket ^_sock;
	Windows::Storage::Streams::DataReader ^_reader;
	Windows::Storage::Streams::DataWriter ^_writer;
	Windows::Foundation::AsyncOperationCompletedHandler<unsigned int> ^_rhandler;
	Windows::Foundation::AsyncOperationCompletedHandler<unsigned int> ^_whandler;
	
	bool Error();
	
	//async handling
	void OnConnect( IAsyncAction ^info,AsyncStatus status );
	void OnLoad( IAsyncOperation<unsigned int> ^info,AsyncStatus status );
	void OnStore( IAsyncOperation<unsigned int> ^info,AsyncStatus status );
};

// ***** tcpstream.cpp *****

using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

BBTcpStream::BBTcpStream():_state(0),_sock( nullptr ),_reader( nullptr ),_writer( nullptr ){
	_revent=CreateEventEx( 0,0,0,EVENT_ALL_ACCESS );
	_wevent=CreateEventEx( 0,0,0,EVENT_ALL_ACCESS );
	_rhandler=ref new AsyncOperationCompletedHandler<unsigned int>( LoadHandler( this ) );	
	_whandler=ref new AsyncOperationCompletedHandler<unsigned int>( StoreHandler( this ) );	
}

BBTcpStream::~BBTcpStream(){
	CloseHandle( _revent );
	CloseHandle( _wevent );
}

bool BBTcpStream::Connect( String addr,int port ){
	if( _state ) return false;

	_sock=ref new StreamSocket();

	auto host=ref new HostName( addr.ToWinRTString() );
	auto service=String( port ).ToWinRTString();

	auto action=_sock->ConnectAsync( host,service );
	action->Completed=ref new AsyncActionCompletedHandler( ConnectHandler( this ) );
	
	if( WaitForSingleObjectEx( _revent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return Error();
	
	if( _state!=1 ) return Error();

	_reader=ref new DataReader( _sock->InputStream );
	_writer=ref new DataWriter( _sock->OutputStream );
	
	return true;
}

void BBTcpStream::OnConnect( IAsyncAction ^info,AsyncStatus status ){
	_state=(status==Windows::Foundation::AsyncStatus::Completed) ? 1 : -1;
	SetEvent( _revent );
}

void BBTcpStream::OnLoad( IAsyncOperation<unsigned int> ^info,AsyncStatus status ){
	SetEvent( _revent );
}

void BBTcpStream::OnStore( IAsyncOperation<unsigned int> ^info,AsyncStatus status ){
	SetEvent( _wevent );
}

int BBTcpStream::ReadAvail(){
	if( _state!=1 ) return 0;
	return 0;
}

int BBTcpStream::WriteAvail(){
	if( _state!=1 ) return 0;
	return 0;
}

int BBTcpStream::Eof(){
	if( _state>0 ) return _state==2;
	return -1;
}

void BBTcpStream::Close(){
	if( _sock ){
		if( _state==1 ) _state=2;
		_sock=nullptr;
		_reader=nullptr;
		_writer=nullptr;
	}
}

bool BBTcpStream::Error(){
	Close();
	_state=-1;
	return false;
}

int BBTcpStream::Read( BBDataBuffer *buffer,int offset,int count ){
	if( _state!=1 ) return 0;

	auto loadop=_reader->LoadAsync( count );
	
	loadop->Completed=_rhandler;
	
	if( WaitForSingleObjectEx( _revent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return Error();
	
	int n=_reader->UnconsumedBufferLength;
	
	if( !n && count ){
		Close();
		return 0;
	}
	
	if( n>count ){
		Print( "Too many bytes to read!" );
		n=count;
	}
		
	unsigned char *p=(unsigned char*)buffer->WritePointer( offset );
	
	_reader->ReadBytes( Platform::ArrayReference<uint8>( (uint8*)p,n ) );

	return n;
}

int BBTcpStream::Write( BBDataBuffer *buffer,int offset,int count ){
	if( _state!=1 ) return 0;
	
	const unsigned char *p=(const unsigned char*)buffer->ReadPointer( offset );
	
	_writer->WriteBytes( Platform::ArrayReference<uint8>( (uint8*)p,count ) );

	auto storeop=_writer->StoreAsync();
	
	storeop->Completed=_whandler;
	
	if( WaitForSingleObjectEx( _wevent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return Error();
	
	return count;
}

#endif

