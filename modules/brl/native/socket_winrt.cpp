
// The gloriously *MAD* winrt version!

#if WINDOWS_8

// ***** socket_winrt.h *****

#include <map>

using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

class BBSocketAddress : public Object{
public:
	HostName ^hostname;
	Platform::String ^service;
	
	BBSocketAddress();

	void Set( BBSocketAddress *address );
	void Set( String host,int port );

	String Host();
	int Port();
	
	bool operator<( const BBSocketAddress &t )const;
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
	bool Open( StreamSocket ^stream );
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

	StreamSocket ^_stream;
	StreamSocketListener ^_server;
	DatagramSocket ^_datagram;
	
	HANDLE _revent;
	HANDLE _wevent;

	DataReader ^_reader;
	DataWriter ^_writer;
	
	AsyncOperationCompletedHandler<unsigned int> ^_recvhandler;
	AsyncOperationCompletedHandler<unsigned int> ^_sendhandler;
	AsyncOperationCompletedHandler<IOutputStream^> ^_getouthandler; 
	
	//for "server" sockets only
	StreamSocket ^_accepted;
	std::vector<StreamSocket^> _acceptQueue;
	int _acceptPut,_acceptGet;
	HANDLE _acceptSema;	

	//for "datagram" sockets only
	typedef DatagramSocketMessageReceivedEventArgs RecvArgs;
	std::vector<RecvArgs^> _recvQueue;
	int _recvPut,_recvGet;
	HANDLE _recvSema;
	
	//for "datagram" sendto
	std::map<BBSocketAddress,DataWriter^> _sendToMap;
	
	template<class X,class Y> struct Delegate{
		BBSocket *socket;
		void (BBSocket::*func)( X,Y );
		Delegate( BBSocket *socket,void (BBSocket::*func)( X,Y ) ):socket( socket ),func( func ){
		}
		void operator()( X x,Y y ){
			(socket->*func)( x,y );
		}
	};
	template<class X,class Y> friend struct Delegate;
	
	template<class X,class Y> Delegate<X,Y> MakeDelegate( void (BBSocket::*func)( X,Y ) ){
		return Delegate<X,Y>( this,func );
	}
	
	template<class X,class Y> TypedEventHandler<X,Y> ^CreateTypedEventHandler( void (BBSocket::*func)( X,Y ) ){
		return ref new TypedEventHandler<X,Y>( Delegate<X,Y>( this,func ) );
	}

	bool Wait( IAsyncAction ^action );
	
	void OnActionComplete( IAsyncAction ^action,AsyncStatus status );
	void OnSendComplete( IAsyncOperation<unsigned int> ^op,AsyncStatus status );
	void OnReceiveComplete( IAsyncOperation<unsigned int> ^op,AsyncStatus status );
	void OnConnectionReceived( StreamSocketListener ^listener,StreamSocketListenerConnectionReceivedEventArgs ^args );
	void OnMessageReceived( DatagramSocket ^socket,DatagramSocketMessageReceivedEventArgs ^args );
	void OnGetOutputStreamComplete( IAsyncOperation<IOutputStream^> ^op,AsyncStatus status );
};

// ***** socket_winrt.cpp *****

BBSocketAddress::BBSocketAddress():hostname( nullptr ),service( nullptr ){
}

void BBSocketAddress::Set( String host,int port ){
	HostName ^hostname=nullptr;
	if( host.Length() ) hostname=ref new HostName( host.ToWinRTString() );
	service=String( port ).ToWinRTString();
}

void BBSocketAddress::Set( BBSocketAddress *address ){
	hostname=address->hostname;
	service=address->service;
}

String BBSocketAddress::Host(){
	return hostname ? hostname->CanonicalName : "0.0.0.0";
}

int BBSocketAddress::Port(){
	return service ? String( service->Data(),service->Length() ).ToInt() : 0;
}

bool BBSocketAddress::operator<( const BBSocketAddress &t )const{
	if( hostname || t.hostname ){
		if( !hostname ) return true;
		if( !t.hostname ) return false;
		int n=HostName::Compare( hostname->CanonicalName,t.hostname->CanonicalName );
		if( n ) return n<0;
	}
	if( service || t.service ){
		if( !service ) return -1;
		if( !t.service ) return 1;
		int n=Platform::String::CompareOrdinal( service,t.service );
		if( n ) return n<0;
	}
	return false;
}

BBSocket::BBSocket(){

	_revent=CreateEventEx( 0,0,0,EVENT_ALL_ACCESS );
	_wevent=CreateEventEx( 0,0,0,EVENT_ALL_ACCESS );
	
	_recvSema=0;
	_acceptSema=0;
	
	_sendhandler=ref new AsyncOperationCompletedHandler<unsigned int>( MakeDelegate( &BBSocket::OnSendComplete ) );
	_recvhandler=ref new AsyncOperationCompletedHandler<unsigned int>( MakeDelegate( &BBSocket::OnReceiveComplete ) );
	_getouthandler=ref new AsyncOperationCompletedHandler<IOutputStream^>( MakeDelegate( &BBSocket::OnGetOutputStreamComplete ) );	
}

BBSocket::~BBSocket(){
	if( _revent ) CloseHandle( _revent );
	if( _wevent ) CloseHandle( _wevent );
	if( _recvSema ) CloseHandle( _recvSema );
	if( _acceptSema ) CloseHandle( _acceptSema );
}

void BBSocket::OnActionComplete( IAsyncAction ^action,AsyncStatus status ){
	SetEvent( _revent );
}

bool BBSocket::Wait( IAsyncAction ^action ){
	action->Completed=ref new AsyncActionCompletedHandler( MakeDelegate( &BBSocket::OnActionComplete ) );
	if( WaitForSingleObjectEx( _revent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return false;
	return action->Status==AsyncStatus::Completed;
}

bool BBSocket::Open( int protocol ){

	switch( protocol ){
	case PROTOCOL_STREAM:
		_stream=ref new StreamSocket();
		return true;
	case PROTOCOL_SERVER:
		_acceptGet=_acceptPut=0;
		_acceptQueue.resize( 256 );
		_acceptSema=CreateSemaphoreEx( 0,0,256,0,0,EVENT_ALL_ACCESS );
		_server=ref new StreamSocketListener();
		_server->ConnectionReceived+=CreateTypedEventHandler( &BBSocket::OnConnectionReceived );
		return true;
	case PROTOCOL_DATAGRAM:
		_recvGet=_recvPut=0;
		_recvQueue.resize( 256 );
		_recvSema=CreateSemaphoreEx( 0,0,256,0,0,EVENT_ALL_ACCESS );
		_datagram=ref new DatagramSocket();
		_datagram->MessageReceived+=CreateTypedEventHandler( &BBSocket::OnMessageReceived );
		return true;
	}

	return false;
}

bool BBSocket::Open( StreamSocket ^stream ){

	_stream=stream;
	
	_reader=ref new DataReader( _stream->InputStream );
	_reader->InputStreamOptions=InputStreamOptions::Partial;
	
	_writer=ref new DataWriter( _stream->OutputStream );
	
	return true;
}

void BBSocket::Close(){
	if( _stream ) delete _stream;
	if( _server ) delete _server;
	if( _datagram ) delete _datagram;
	_stream=nullptr;
	_server=nullptr;
	_datagram=nullptr;
}

bool BBSocket::Bind( String host,int port ){

	HostName ^hostname=nullptr;
	if( host.Length() ) hostname=ref new HostName( host.ToWinRTString() );
	auto service=(port ? String( port ) : String()).ToWinRTString();

	if( _stream ){
//		return Wait( _stream->BindEndpointAsync( hostname,service ) );
	}else if( _server ){
		return Wait( _server->BindEndpointAsync( hostname,service ) );
	}else if( _datagram ){
		return Wait( _datagram->BindEndpointAsync( hostname,service ) );
	}

	return false;
}

bool BBSocket::Listen( int backlog ){
	return _server!=nullptr;
}

bool BBSocket::Accept(){
	if( WaitForSingleObjectEx( _acceptSema,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return false;
	_accepted=_acceptQueue[_acceptGet & 255];
	_acceptQueue[_acceptGet++ & 255]=nullptr;
	return true;
}

BBSocket *BBSocket::Accepted(){
	BBSocket *socket=new BBSocket();
	if( socket->Open( _accepted ) ) return socket;
	return 0;
}

void BBSocket::OnConnectionReceived( StreamSocketListener ^listener,StreamSocketListenerConnectionReceivedEventArgs ^args ){

	_acceptQueue[_acceptPut++ & 255]=args->Socket;
	ReleaseSemaphore( _acceptSema,1,0 );
}

void BBSocket::OnMessageReceived( DatagramSocket ^socket,DatagramSocketMessageReceivedEventArgs ^args ){

	_recvQueue[_recvPut++ & 255]=args;
	ReleaseSemaphore( _recvSema,1,0 );
}

bool BBSocket::Connect( String host,int port ){

	auto hostname=ref new HostName( host.ToWinRTString() );
	auto service=String( port ).ToWinRTString();

	if( _stream ){

		if( !Wait( _stream->ConnectAsync( hostname,service ) ) ) return false;
		
		_reader=ref new DataReader( _stream->InputStream );
		_reader->InputStreamOptions=InputStreamOptions::Partial;

		_writer=ref new DataWriter( _stream->OutputStream );
	
		return true;
		
	}else if( _datagram ) {
	
		if( !Wait( _datagram->ConnectAsync( hostname,service ) ) ) return false;
		
		_writer=ref new DataWriter( _datagram->OutputStream );
		
		return true;
	}
}

int BBSocket::Send( BBDataBuffer *data,int offset,int count ){

	if( !_writer ) return 0;

	const unsigned char *p=(const unsigned char*)data->ReadPointer( offset );
	
	_writer->WriteBytes( Platform::ArrayReference<uint8>( (uint8*)p,count ) );
	auto op=_writer->StoreAsync();
	op->Completed=_sendhandler;
	
	if( WaitForSingleObjectEx( _wevent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return 0;

//	if( op->Status!=AsyncStatus::Completed ) return 0;
	
	return count;
}

void BBSocket::OnSendComplete( IAsyncOperation<unsigned int> ^op,AsyncStatus status ){

	SetEvent( _wevent );
}

int BBSocket::Receive( BBDataBuffer *data,int offset,int count ){

	if( _stream ){
	
		auto op=_reader->LoadAsync( count );
		op->Completed=_recvhandler;
	
		if( WaitForSingleObjectEx( _revent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return 0;
		
	//	if( op->Status!=AsyncStatus::Completed ) return 0;
		
		int n=_reader->UnconsumedBufferLength;
			
		_reader->ReadBytes( Platform::ArrayReference<uint8>( (uint8*)data->WritePointer( offset ),n ) );
	
		return n;
		
	}else if( _datagram ){

		if( WaitForSingleObjectEx( _recvSema,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return 0;
		
		auto recvArgs=_recvQueue[_recvGet & 255];
		_recvQueue[_recvGet++ & 255]=nullptr;
		
		auto reader=recvArgs->GetDataReader();
		int n=reader->UnconsumedBufferLength;
		if( n>count ) n=count;

		reader->ReadBytes( Platform::ArrayReference<uint8>( (uint8*)data->WritePointer( offset ),n ) );
		
		return n;
	}
	return 0;
}

void BBSocket::OnReceiveComplete( IAsyncOperation<unsigned int> ^op,AsyncStatus status ){

	SetEvent( _revent );
}

int BBSocket::SendTo( BBDataBuffer *data,int offset,int count,BBSocketAddress *address ){

	auto it=_sendToMap.find( *address );
	
	if( it==_sendToMap.end() ){
	
		auto op=_datagram->GetOutputStreamAsync( address->hostname,address->service );
		op->Completed=_getouthandler;
		
		if( WaitForSingleObjectEx( _wevent,INFINITE,FALSE )!=WAIT_OBJECT_0 || op->Status!=AsyncStatus::Completed ){
			bbPrint( "GetOutputStream failed" );
			return 0;
		}	
		it=_sendToMap.insert( std::make_pair( *address,ref new DataWriter( op->GetResults() ) ) ).first;
	}

	auto writer=it->second;

	writer->WriteBytes( Platform::ArrayReference<uint8>( (uint8*)data->ReadPointer( offset ),count ) );
	auto op=writer->StoreAsync();
	op->Completed=_sendhandler;
	
	if( WaitForSingleObjectEx( _wevent,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return 0;

//	if( op->Status!=AsyncStatus::Completed ) return 0;
	
	return count;
}

void BBSocket::OnGetOutputStreamComplete( IAsyncOperation<IOutputStream^> ^op,AsyncStatus status ){

	SetEvent( _wevent );
}

int BBSocket::ReceiveFrom( BBDataBuffer *data,int offset,int count,BBSocketAddress *address ){

	if( !_datagram ) return 0;
	
	if( WaitForSingleObjectEx( _recvSema,INFINITE,FALSE )!=WAIT_OBJECT_0 ) return 0;
	
	auto recvArgs=_recvQueue[_recvGet & 255];
	_recvQueue[_recvGet++ & 255]=nullptr;
	
	auto reader=recvArgs->GetDataReader();
	int n=reader->UnconsumedBufferLength;
	if( n>count ) n=count;

	reader->ReadBytes( Platform::ArrayReference<uint8>( (uint8*)data->WritePointer( offset ),n ) );

	address->hostname=recvArgs->RemoteAddress;
	address->service=recvArgs->RemotePort;
	
	return n;
}
	
void BBSocket::GetLocalAddress( BBSocketAddress *address ){
	if( _stream ){
		address->hostname=_stream->Information->LocalAddress;
		address->service=_stream->Information->LocalPort;
	}else if( _server ){
		address->hostname=nullptr;
		address->service=_server->Information->LocalPort;
	}else if( _datagram ){
		address->hostname=_datagram->Information->LocalAddress;
		address->service=_datagram->Information->LocalPort;
	}
}

void BBSocket::GetRemoteAddress( BBSocketAddress *address ){
	if( _stream ){
		address->hostname=_stream->Information->RemoteAddress;
		address->service=_stream->Information->RemotePort;
	}else if( _server ){
		address->hostname=nullptr;
		address->service=nullptr;
	}else if( _datagram ){
		address->hostname=_datagram->Information->RemoteAddress;
		address->service=_datagram->Information->RemotePort;
	}
}

#endif
