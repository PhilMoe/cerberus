
#if WINDOWS_8

// ***** socket_win8.h *****

class BBSocketAddress : public Object{
public:
	void Set( BBSocketAddress *addres );
	void Set( String host,int port );
	String Host();
	int Port();
};

class BBSocket : public Object{
public:
	enum{
		PROTOCOL_STREAM=1,
		PROTOCOL_SERVER=2,
		PROTOCOL_DATAGRAM=3
	};

	BBSocket();
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

private:

	bool _connected;
	HANDLE _revent,_wevent;
	Windows::Networking::Sockets::StreamSocket ^_stream;
	Windows::Networking::Sockets::StreamSocketListener ^_server;
	Windows::Networking::Sockets::DatagramSocket ^_datagram;
	
	Windows::Storage::Streams::DataReader ^_reader;
	Windows::Storage::Streams::DataWriter ^_writer;
	
	Windows::Foundation::AsyncOperationCompletedHandler<unsigned int> ^_recvhandler;
	Windows::Foundation::AsyncOperationCompletedHandler<unsigned int> ^_sendhandler;
};

// ***** socket_win8.cpp *****

using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

struct EventSetter{
	HANDLE event;
	
	EventSetter( HANDLE event ):event( event ){
	}
	
	void operator()( IAsyncAction ^info,AsyncStatus status ){
		SetEvent( event );
	}
	
	void operator()( IAsyncOperation<unsigned int> ^info,AsyncStatus status ){
		SetEvent( event );
	}
};

BBSocket::BBSocket():_stream( nullptr ),_server( nullptr ),_datagram( nullptr ),_reader( nullptr ),_writer( nullptr ){
	_revent=CreateEventEx( 0,0,0,EVENT_ALL_ACCESS );
	_wevent=CreateEventEx( 0,0,0,EVENT_ALL_ACCESS );
	_recvhandler=ref new AsyncOperationCompletedHandler<unsigned int>( EventSetter( _revent ) );
	_sendhandler=ref new AsyncOperationCompletedHandler<unsigned int>( EventSetter( _wevent ) );
}

BBSocket::~BBSocket(){
	CloseHandle( _revent );
	CloseHandle( _wevent );
}

bool BBSocket::Open( int protocol ){

	switch( protocol ){
	case PROTOCOL_STREAM:
		_stream=ref new StreamSocket();
		return true;
	}

	return false;
}

void BBSocket::Close(){
	if( _stream ){
		_stream=nullptr;
		_server=nullptr;
		_datagram=nullptr;
		_reader=nullptr;
		_writer=nullptr;
	}
}

bool BBSocket::Bind( String host,int port ){

	if( _stream ){
	}else if( _server ){
	}else if( _datagram ){
	}

	return false;
}

bool BBSocket::Connect( String host,int port ){

	auto hostname=ref new HostName( host.ToWinRTString() );
	auto service=String( port ).ToWinRTString();

	auto action=_stream->ConnectAsync( hostname,service );
	
	action->Completed=ref new AsyncActionCompletedHandler( EventSetter( _revent ) );
	
	if( WaitForSingleObjectEx( _revent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return false;
	
	if( action->Status!=Windows::Foundation::AsyncStatus::Completed ) return false;

	_reader=ref new DataReader( _stream->InputStream );
	_writer=ref new DataWriter( _stream->OutputStream );
	
	return true;
}

bool BBSocket::Listen( int backlog ){
	return false;
}

bool BBSocket::Accept(){
	return false;
}

BBSocket *BBSocket::Accepted(){
	return 0;
}

int BBSocket::Send( BBDataBuffer *buffer,int offset,int count ){
	
	const unsigned char *p=(const unsigned char*)buffer->ReadPointer( offset );
	
	_writer->WriteBytes( Platform::ArrayReference<uint8>( (uint8*)p,count ) );

	auto operation=_writer->StoreAsync();
	
	operation->Completed=_sendhandler;
	
	if( WaitForSingleObjectEx( _wevent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return 0;
	
	return count;
}

int BBSocket::Receive( BBDataBuffer *buffer,int offset,int count ){

	auto operation=_reader->LoadAsync( count );

	operation->Completed=_recvhandler;

	if( WaitForSingleObjectEx( _revent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return 0;
	
	int n=_reader->UnconsumedBufferLength;
		
	unsigned char *p=(unsigned char*)buffer->WritePointer( offset );
	
	_reader->ReadBytes( Platform::ArrayReference<uint8>( (uint8*)p,n ) );

	return n;
}

int BBSocket::SendTo( BBDataBuffer *data,int offset,int count,BBSocketAddress *address ){

	return 0;
}

int BBSocket::ReceiveFrom( BBDataBuffer *data,int offset,int count,BBSocketAddress *address ){

	return 0;
}
	
void BBSocket::GetLocalAddress( BBSocketAddress *address ){
}

void BBSocket::GetRemoteAddress( BBSocketAddress *address ){
}

#endif
