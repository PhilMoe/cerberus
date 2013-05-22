
// ***** TcpServer.h ******

class BBTcpServer : public Object{

	public:

	BBTcpServer();
	~BBTcpServer();
	
	virtual bool Create( int port );

	virtual int Port();
	
	virtual void Listen( int backlog );
	
	virtual BBTcpStream *Accept();
	
	virtual void Close();
	
	private:
	
	int _sock;
	bool _listening;
	int _port;
};

// ***** TcpServer.cpp *****

BBTcpServer::BBTcpServer():_sock( -1 ),_port( 0 ),_listening( false ){
	BBTcpStream::InitSockets();
}

BBTcpServer::~BBTcpServer(){
	if( _sock>=0 ) closesocket( _sock );
}

bool BBTcpServer::Create( int port ){
	if( _sock>=0 ) return false;
	
	_sock=socket( AF_INET,SOCK_STREAM,IPPROTO_TCP );
	if( _sock<0 ) return false;

	struct sockaddr_in sa;
	memset( &sa,0,sizeof(sa) );
	sa.sin_family=AF_INET;
	sa.sin_addr.s_addr=htonl( INADDR_ANY );
	sa.sin_port=htons( port );
	
	if( bind( _sock,(const sockaddr*)&sa,sizeof(sa) )<0 ){
		closesocket( _sock );
		return false;
	}
	
	int size=sizeof(sa);
	getsockname( _sock,(sockaddr*)&sa,&size );
	_port=htons( sa.sin_port );
	
	return true;
}
	
int BBTcpServer::Port(){
	return _port;
}
	
void BBTcpServer::Listen( int backlog ){
	if( _sock<0 || _listening ) return;
	
	listen( _sock,backlog );
	
	_listening=true;
}

BBTcpStream *BBTcpServer::Accept(){
	if( _sock<0 ) return 0;
	
	fd_set r_set;
	FD_ZERO( &r_set );
	FD_SET( _sock,&r_set );
	
	timeval tv;	
	memset( &tv,0,sizeof( tv ) );

	if( select( _sock+1,&r_set,0,0,&tv )!=1 ) return 0;
	
	int sock=accept( _sock,0,0 );
	if( sock>=0 ) return new BBTcpStream( sock );
	
	return 0;
}

void BBTcpServer::Close(){
	if( _sock<0 ) return;
	closesocket( _sock );
	_sock=-1;
	_port=0;
	_listening=false;
}
