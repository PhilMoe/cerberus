
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
	sockaddr_storage _sa;
	int _len;
	
	BBSocketAddress();
	
	void Set( String host,int port );
	void Set( BBSocketAddress *address );
	void Set( const sockaddr_storage &sa,int len );
	
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

static int setsockaddr( sockaddr_storage *sa,String host,int port ){

	memset( sa,0,sizeof(*sa) );
	
	if( host.Length()>1023 ) return 0;
	
	char hostname[1024],*hostp=0;
	if( host.Length() ){
		strcpy( hostname,host.ToCString<char>() );
		hostp=hostname;
	}

	char service[80];
	sprintf( service,"%i",port );
	
	addrinfo hints;

	memset( &hints,0,sizeof( hints ) );
	hints.ai_family=AF_UNSPEC;
	hints.ai_flags=AI_DEFAULT;
	if( !hostp ) hints.ai_flags|=AI_PASSIVE;

	addrinfo *res;
	if( getaddrinfo( hostp,service,&hints,&res )<0 ) return 0;
	if( !res ) return 0;
	
	int len=res->ai_addrlen;
	memcpy( sa,res->ai_addr,len );
	
	freeaddrinfo( res );
	
	return len;
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
	_len=0;
}

void BBSocketAddress::Set( String host,int port ){
	_len=setsockaddr( &_sa,host,port );
	_valid=false;
}

void BBSocketAddress::Set( BBSocketAddress *address ){
	_sa=address->_sa;
	_valid=false;
}

void BBSocketAddress::Set( const sockaddr_storage &sa,int len ){
	_sa=sa;
	_len=len;
	_valid=false;
}

void BBSocketAddress::Validate(){
	if( _valid ) return;
	char buf[1024];
	if( inet_ntop( AF_INET6,&_sa,buf,1024 ) ){
		_host=String( buf );
	}else{
		_host=String( "?????" );
	}
	_port=htons( ((sockaddr_in6&)_sa).sin6_port );
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
	
		//Create socket later, once we know hostname...
		_proto=proto;
		return true;
		
	case PROTOCOL_SERVER:
	
		_sock=socket( PF_INET6,SOCK_STREAM,IPPROTO_TCP );
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
		_sock=socket( PF_INET6,SOCK_DGRAM,IPPROTO_UDP );
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

	sockaddr_storage sa;
	socklen_t size=sizeof(sa);
	memset( &sa,0,size );
	
	getsockname( _sock,(sockaddr*)&sa,&size );
	address->Set( sa,size );
}

void BBSocket::GetRemoteAddress( BBSocketAddress *address ){

	sockaddr_storage sa;
	socklen_t size=sizeof(sa);
	memset( &sa,0,size );
	
	getpeername( _sock,(sockaddr*)&sa,&size );
	address->Set( sa,size );
}

bool BBSocket::Connect( String host,int port ){

	if( _sock>=0 ) return false;
	
	if( host.Length()>1023 ) return false;
	
	char hostname[1024];
	strcpy( hostname,host.ToCString<char>() );
	
	char service[80];
	sprintf( service,"%i",port );
	
	addrinfo hints;
	memset( &hints,0,sizeof( hints ) );

	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_DEFAULT;

	addrinfo *addrs=0;
	if( getaddrinfo( hostname,service,&hints,&addrs ) ) return -1;
	
	for( addrinfo *res=addrs;res;res=res->ai_next ){
	
		_sock=socket( res->ai_family,res->ai_socktype,res->ai_protocol );
		if( _sock<0 ) continue;
		
		if( !connect( _sock,res->ai_addr,res->ai_addrlen ) ) break;
			
		close( _sock );
		_sock=-1;
	}
	
	freeaddrinfo( addrs );
	
	if( _sock<0 ) return false;
	
	int nodelay=1;
	setsockopt( _sock,IPPROTO_TCP,TCP_NODELAY,(const char*)&nodelay,sizeof(nodelay) );
	
	return true;
}

bool BBSocket::Bind( String host,int port ){
	sockaddr_storage sa;
	int len=setsockaddr( &sa,host,port );
	return bind( _sock,(sockaddr*)&sa,len )>=0;
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
	sockaddr_storage sa;
	socklen_t size=sizeof(sa);
	memset( &sa,0,size );
	int n=recvfrom( _sock,(char*)data->WritePointer( offset ),count,0,(sockaddr*)&sa,&size );
	address->Set( sa,size );
	return n;
}

#endif
