
#if !WINDOWS_8

// ***** socket.h *****

#if WINDOWS_PHONE_8

#include <Winsock2.h>

typedef int socklen_t;

#elif _WIN32

#include <winsock.h>

typedef int socklen_t;

#else

#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define closesocket close
#define ioctlsocket ioctl

#endif

class BBSocketAddress : public Object{
public:
	sockaddr_in _sa;
	
	BBSocketAddress();
	
	void Set( String host,int port );
	void Set( BBSocketAddress *address );
	void Set( const sockaddr_in &sa );
	
	String Host(){ Validate();return _host; }
	int Port(){ Validate();return _port; }
	
private:
	bool _valid;
	String _host;
	int _port;
	
	void Validate();
};

class BBSocket : public Object{
public:
	enum{
		PROTOCOL_CLIENT=1,
		PROTOCOL_SERVER=2,
		PROTOCOL_DATAGRAM=3
	};
	
	BBSocket();
	BBSocket( int sock );
	~BBSocket();
	
	bool Open( int protocol );
	void Close();
	
	bool Bind( String host,int port );
	bool Connect( String host,int port );
	bool Listen( int backlog );
	bool Accept();
	BBSocket *Accepted();

	int Send( BBDataBuffer *data,int offset,int count );
	int Receive( BBDataBuffer *data,int offset,int count );

	int SendTo( BBDataBuffer *data,int offset,int count,BBSocketAddress *address );
	int ReceiveFrom( BBDataBuffer *data,int offset,int count,BBSocketAddress *address );
	
	void GetLocalAddress( BBSocketAddress *address );
	void GetRemoteAddress( BBSocketAddress *address );
	
	static void InitSockets();
	
protected:
	int _sock;
	int _proto;
	int _accepted;
};

// ***** socket.cpp *****

static void setsockaddr( sockaddr_in *sa,String host,int port ){
	memset( sa,0,sizeof(*sa) );
	sa->sin_family=AF_INET;
	sa->sin_port=htons( port );
	sa->sin_addr.s_addr=htonl( INADDR_ANY );
	
	if( host.Length() && host.Length()<1024 ){
		char buf[1024];
		for( int i=0;i<host.Length();++i ) buf[i]=host[i];
		buf[host.Length()]=0;
		if( hostent *host=gethostbyname( buf ) ){
			if( char *hostip=inet_ntoa(*(in_addr *)*host->h_addr_list) ){
				sa->sin_addr.s_addr=inet_addr( hostip );
			}
		}
	}
}

void BBSocket::InitSockets(){
#if _WIN32
	static bool started;
	if( !started ){
		WSADATA ws;
		WSAStartup( 0x101,&ws );
		started=true;
	}
#endif
}

BBSocketAddress::BBSocketAddress():_valid( false ){
	BBSocket::InitSockets();
	memset( &_sa,0,sizeof(_sa) );
	_sa.sin_family=AF_INET;
}

void BBSocketAddress::Set( String host,int port ){
	setsockaddr( &_sa,host,port );
	_valid=false;
}

void BBSocketAddress::Set( BBSocketAddress *address ){
	_sa=address->_sa;
	_valid=false;
}

void BBSocketAddress::Set( const sockaddr_in &sa ){
	_sa=sa;
	_valid=false;
}

void BBSocketAddress::Validate(){
	if( _valid ) return;
	_host=String( int(_sa.sin_addr.s_addr)&0xff )+"."+String( int(_sa.sin_addr.s_addr>>8)&0xff )+"."+String( int(_sa.sin_addr.s_addr>>16)&0xff )+"."+String( int(_sa.sin_addr.s_addr>>24)&0xff );
	_port=htons( _sa.sin_port );
	_valid=true;
}

BBSocket::BBSocket():_sock( -1 ){
	BBSocket::InitSockets();
}

BBSocket::BBSocket( int sock ):_sock( sock ){
}

BBSocket::~BBSocket(){

	if( _sock>=0 ) closesocket( _sock );
}

bool BBSocket::Open( int proto ){

	if( _sock>=0 ) return false;
	
	switch( proto ){
	case PROTOCOL_CLIENT:
	case PROTOCOL_SERVER:
		_sock=socket( AF_INET,SOCK_STREAM,IPPROTO_TCP );
		if( _sock>=0 ){
			//nodelay
			int nodelay=1;
			setsockopt( _sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nodelay,sizeof(nodelay) );
	
			//Do this on Mac so server ports can be quickly reused...
			#if __APPLE__ || __linux
			int flag=1;
			setsockopt( _sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag) );
			#endif
		}
		break;
	case PROTOCOL_DATAGRAM:
		_sock=socket( AF_INET,SOCK_DGRAM,IPPROTO_UDP );
		break;
	}
	
	if( _sock<0 ) return false;
	
	_proto=proto;
	return true;
}

void BBSocket::Close(){
	if( _sock<0 ) return;
	closesocket( _sock );
	_sock=-1;
}

void BBSocket::GetLocalAddress( BBSocketAddress *address ){
	sockaddr_in sa;
	memset( &sa,0,sizeof(sa) );
	sa.sin_family=AF_INET;
	socklen_t size=sizeof(sa);
	if( _sock>=0 ) getsockname( _sock,(sockaddr*)&sa,&size );
	address->Set( sa );
}

void BBSocket::GetRemoteAddress( BBSocketAddress *address ){
	sockaddr_in sa;
	memset( &sa,0,sizeof(sa) );
	sa.sin_family=AF_INET;
	socklen_t size=sizeof(sa);
	if( _sock>=0 ) getpeername( _sock,(sockaddr*)&sa,&size );
	address->Set( sa );
}

bool BBSocket::Bind( String host,int port ){

	sockaddr_in sa;
	setsockaddr( &sa,host,port );
	
	return bind( _sock,(sockaddr*)&sa,sizeof(sa) )>=0;
}

bool BBSocket::Connect( String host,int port ){

	sockaddr_in sa;
	setsockaddr( &sa,host,port );
	
	return connect( _sock,(sockaddr*)&sa,sizeof(sa) )>=0;
}

bool BBSocket::Listen( int backlog ){
	return listen( _sock,backlog )>=0;
}

bool BBSocket::Accept(){
	_accepted=accept( _sock,0,0 );
	return _accepted>=0;
}

BBSocket *BBSocket::Accepted(){
	if( _accepted>=0 ) return new BBSocket( _accepted );
	return 0;
}

int BBSocket::Send( BBDataBuffer *data,int offset,int count ){
	return send( _sock,(const char*)data->ReadPointer(offset),count,0 );
}

int BBSocket::Receive( BBDataBuffer *data,int offset,int count ){
	return recv( _sock,(char*)data->WritePointer( offset ),count,0 );
}

int BBSocket::SendTo( BBDataBuffer *data,int offset,int count,BBSocketAddress *address ){
	return sendto( _sock,(const char*)data->ReadPointer(offset),count,0,(sockaddr*)&address->_sa,sizeof(address->_sa) );
}

int BBSocket::ReceiveFrom( BBDataBuffer *data,int offset,int count,BBSocketAddress *address ){
	sockaddr_in sa;
	socklen_t size=sizeof(sa);
	memset( &sa,0,size );
	int n=recvfrom( _sock,(char*)data->WritePointer( offset ),count,0,(sockaddr*)&sa,&size );
	address->Set( sa );
	return n;
}

#endif
