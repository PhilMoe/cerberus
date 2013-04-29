
// ***** tcpstream.h *****

#if WINDOWS_PHONE_8

#include <Winsock2.h>

#elif _WIN32

#include <winsock.h>

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
	~BBTcpStream();
	
	bool Connect( String addr,int port );
	int ReadAvail();
	int WriteAvail();
	
	int Eof();
	void Close();
	int Read( BBDataBuffer *buffer,int offset,int count );
	int Write( BBDataBuffer *buffer,int offset,int count );
	
private:
	int _sock;
	int _state;	//0=INIT, 1=CONNECTED, 2=CLOSED, -1=ERROR
};

// ***** tcpstream.cpp *****

BBTcpStream::BBTcpStream():_sock(-1),_state(0){
#if _WIN32
	static bool started;
	if( !started ){
		WSADATA ws;
		WSAStartup( 0x101,&ws );
		started=true;
	}
#endif
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
/*				
					int rcvbuf=16384;
					if( setsockopt( _sock,SOL_SOCKET,SO_RCVBUF,(const char*)&rcvbuf,sizeof(rcvbuf) )<0 ){
						puts( "setsockopt failed!" );
					}
*/
				
					int nodelay=1;
					if( setsockopt( _sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nodelay,sizeof(nodelay) )<0 ){
						puts( "setsockopt failed!" );
					}

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

/* TODO!!!!!

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

	//async handling
	void OnConnect( IAsyncAction ^info,AsyncStatus status );
	void OnLoad( IAsyncOperation<unsigned int> ^info,AsyncStatus status );
	void OnStore( IAsyncOperation<unsigned int> ^info,AsyncStatus status );
	
private:

	int _state;	//0=INIT, 1=CONNECTED, 2=CLOSED, -1=ERROR

	Windows::Networking::Sockets::StreamSocket ^_sock;
	HANDLE _event;
	Windows::Storage::Streams::DataReader ^_reader;
	Windows::Storage::Streams::DataWriter ^_writer;
};

// ***** tcpstream.cpp *****

using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

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

BBTcpStream::BBTcpStream():_state(0),_sock(nullptr),_event(nullptr),_reader(nullptr),_writer(nullptr){
}

BBTcpStream::~BBTcpStream(){
	Close();
}

bool BBTcpStream::Connect( String addr,int port ){

	if( _state ) return false;

	_sock=ref new StreamSocket();
	_event=CreateEventEx( 0,0,CREATE_EVENT_MANUAL_RESET,EVENT_ALL_ACCESS );
	
	auto host=ref new HostName( addr.ToWinRTString() );
	auto service=String( port ).ToWinRTString();

	auto action=_sock->ConnectAsync( host,service );
	
	action->Completed=ref new AsyncActionCompletedHandler( ConnectHandler( this ) );
	
	Print( "Connecting..." );
	if( WaitForSingleObjectEx( _event,INFINITE,FALSE )!=WAIT_OBJECT_0 ){
		_state=-1;
		Close();
		return false;
	}
	ResetEvent( _event );
	Print( "Connect done" );
	
	if( _state==1 ){
		_reader=ref new DataReader( _sock->InputStream );
		_writer=ref new DataWriter( _sock->OutputStream );
		return true;
	}
	
	_state=-1;
	Close();
	return false;
}

void BBTcpStream::OnConnect( IAsyncAction ^info,AsyncStatus status ){
	if( status==Windows::Foundation::AsyncStatus::Completed ){
		_state=1;
	}else{
		_state=-1;
	}
	SetEvent( _event );
}

void BBTcpStream::OnLoad( IAsyncOperation<unsigned int> ^info,AsyncStatus status ){
	SetEvent( _event );
}

void BBTcpStream::OnStore( IAsyncOperation<unsigned int> ^info,AsyncStatus status ){
	SetEvent( _event );
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
	if( !_sock ) return;
	if( _state==1 ) _state=2;
	CloseHandle( _event );
	_reader=nullptr;
	_writer=nullptr;
	_event=nullptr;
	_sock=nullptr;
}

int BBTcpStream::Read( BBDataBuffer *buffer,int offset,int count ){
	if( _state!=1 ) return 0;

//	if( _reader->UnconsumedBufferLength<count ){

		auto loadop=_reader->LoadAsync( count );	//-_reader->UnconsumedBufferLength );
		
		loadop->Completed=ref new AsyncOperationCompletedHandler<unsigned int>( LoadHandler( this ) );
		
		Print( "Reading..." );
		if( WaitForSingleObjectEx( _event,INFINITE,FALSE )!=WAIT_OBJECT_0 ){
			_state=-1;
			Close();
			return 0;
		}
		ResetEvent( _event );
		Print( "Read done" );
//	}
	
	int n=_reader->UnconsumedBufferLength;
	if( n>count ){
		Print( "Too many bytes to read!" );
		n=count;
	}
		
	unsigned char *p=(unsigned char*)buffer->WritePointer( offset );
		
	for( int i=0;i<n;++i ){
		*p++=_reader->ReadByte();
	}
	
	if( n>0 || !count ) return n;
	
	Close();
	
	return n;
}

int BBTcpStream::Write( BBDataBuffer *buffer,int offset,int count ){
	if( _state!=1 ) return 0;
	
	const unsigned char *p=(const unsigned char*)buffer->ReadPointer( offset );
	
	for( int i=0;i<count;++i ){
		_writer->WriteByte( *p++ );
	}
	
	auto storeop=_writer->StoreAsync();
	
	storeop->Completed=ref new AsyncOperationCompletedHandler<unsigned int>( StoreHandler( this ) );
	
	Print( "Writing..." );
	if( WaitForSingleObjectEx( _event,INFINITE,FALSE )!=WAIT_OBJECT_0 ){
		Print( "Here!" );
		_state=-1;
		Close();
		return 0;
	}
	ResetEvent( _event );
	Print( "Write done" );
	
	return count;
}
*/
