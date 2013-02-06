
// ***** tcpstream.h *****

#if _WIN32

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
	printf( "n=%i\n",n );fflush( stdout );
	if( n>=0 ) return n;
	_state=-1;
	return 0;
}
