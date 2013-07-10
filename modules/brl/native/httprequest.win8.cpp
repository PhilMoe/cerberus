
// ***** HttpRequest.h *****

#include <msxml6.h>
#include <wrl/client.h>
#include <wrl/implements.h>

//Ugh - seems to be how you're supposed to send strings?...
//
class CXMLHTTPRequestData : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>,ISequentialStream>{

public:

	STDMETHODIMP Read( _Out_writes_bytes_to_(cb,*pcbRead) void *pv,ULONG cb,_Out_opt_  ULONG *pcbRead );
    STDMETHODIMP Write( _In_reads_bytes_(cb) const void *pv,ULONG cb,_Out_opt_ ULONG *pcbWritten );

	int SetText( String data );

private:

    friend Microsoft::WRL::ComPtr<CXMLHTTPRequestData> Microsoft::WRL::Details::Make<CXMLHTTPRequestData>();
    
	std::vector<unsigned char> _buf;
	int _ptr;
	
	CXMLHTTPRequestData();    
};

class CXMLHTTPRequest2Callback : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>,IXMLHTTPRequest2Callback>{

public:

	STDMETHODIMP OnDataAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_opt ISequentialStream *pResponseStream );
	STDMETHODIMP OnError( __RPC__in_opt IXMLHTTPRequest2 *pXHR,HRESULT hrError );
	STDMETHODIMP OnHeadersAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR,DWORD dwStatus,__RPC__in_string const WCHAR *pwszStatus );
	STDMETHODIMP OnRedirect( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_string const WCHAR *pwszRedirectUrl );
	STDMETHODIMP OnResponseReceived( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_opt ISequentialStream *pResponseStream );
	
	bool Wait();
	
	String ResponseText(){ return _response; }
	int Status(){ return _status; }
	int BytesReceived(){ return _recv; }
	
private:

    friend HRESULT Microsoft::WRL::Details::MakeAndInitialize<CXMLHTTPRequest2Callback,CXMLHTTPRequest2Callback>( CXMLHTTPRequest2Callback** );

	HANDLE _event;
	String _response;
	int _status;
	int _recv;
	
	CXMLHTTPRequest2Callback();
	~CXMLHTTPRequest2Callback();

    STDMETHODIMP RuntimeClassInitialize();
};

class BBHttpRequest : public BBThread{

public:

	BBHttpRequest();
		
	void Open( String req,String url );
	void SetHeader( String name,String value );
	void Send();
	void SendText( String text,String encoding );
	String ResponseText();
	int Status();
	int BytesReceived();
	
private:
	
	Microsoft::WRL::ComPtr<IXMLHTTPRequest2> _req;
	Microsoft::WRL::ComPtr<CXMLHTTPRequestData> _data;
	Microsoft::WRL::ComPtr<CXMLHTTPRequest2Callback> _cb;
	
	void Run__UNSAFE__();
};

// ***** HttpRequest.cpp *****

CXMLHTTPRequestData::CXMLHTTPRequestData():_ptr(0){
}

STDMETHODIMP CXMLHTTPRequestData::Read( _Out_writes_bytes_to_(cb,*pcbRead) void *pv,ULONG cb,_Out_opt_ ULONG *pcbRead ){
	int n=_buf.size()-_ptr;
	if( n<cb ) cb=n;
	if( n ){
		memcpy( pv,&_buf[_ptr],n );
		_ptr+=n;
	}
	if( pcbRead ) *pcbRead=n;
	return 0;
}

STDMETHODIMP CXMLHTTPRequestData::Write( _In_reads_bytes_(cb) const void *pv,ULONG cb,_Out_opt_ ULONG *pcbWritten ){
	if( pcbWritten ) *pcbWritten=0;
	return 0;
}

int CXMLHTTPRequestData::SetText( String data ){
	_buf.clear();
	data.Save( _buf );
	_ptr=0;
	return _buf.size();
}

CXMLHTTPRequest2Callback::CXMLHTTPRequest2Callback():_event( 0 ),_response( "" ),_status( -1 ),_recv( 0 ){
}

CXMLHTTPRequest2Callback::~CXMLHTTPRequest2Callback(){

	if( _event ) CloseHandle( _event );
}

STDMETHODIMP CXMLHTTPRequest2Callback::RuntimeClassInitialize(){

	Print( "RuntimeClassInitialize" );

	_event=CreateEventEx( 0,0,CREATE_EVENT_MANUAL_RESET,EVENT_ALL_ACCESS );
	
	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnDataAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_opt ISequentialStream *pResponseStream ){

	Print( "Data" );

	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnError( __RPC__in_opt IXMLHTTPRequest2 *pXHR,HRESULT hrError ){

	Print( "Error" );
	
	_status=-1;

	SetEvent( _event );

	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnHeadersAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR,DWORD dwStatus,__RPC__in_string const WCHAR *pwszStatus ){

	Print( "Headers" );
	
	_status=dwStatus;

	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnRedirect( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_string const WCHAR *pwszRedirectUrl ){

	Print( "Redirect" );
	
	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnResponseReceived( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_opt ISequentialStream *pStream ){

	Print( "Response" );

	if( pStream ){

		const int BUF_SZ=4096;
		std::vector<void*> tmps;
		int length=0;
		
		for(;;){
			void *p=malloc( BUF_SZ );
			ULONG n=0;
			pStream->Read( p,BUF_SZ,&n );
			tmps.push_back( p );
			length+=n;
			if( n!=BUF_SZ ) break;
		}
		
		unsigned char *data=(unsigned char*)malloc( length );
		unsigned char *p=data;
		
		int sz=length;
		for( int i=0;i<tmps.size();++i ){
			int n=sz>BUF_SZ ? BUF_SZ : sz;
			memcpy( p,tmps[i],n );
			free( tmps[i] );
			sz-=n;
			p+=n;
		}
		
		_response=String::Load( data,length );
		
		_recv=length;
		
		free( data );
	}
	
	SetEvent( _event );

	return 0;
}

bool CXMLHTTPRequest2Callback::Wait(){

	Print( "Callback::Wait!" );

	return WaitForSingleObjectEx( _event,INFINITE,FALSE )==WAIT_OBJECT_0;
}

BBHttpRequest::BBHttpRequest(){
}

void BBHttpRequest::Open( String req,String url ){
	
	if( FAILED( CoCreateInstance( CLSID_FreeThreadedXMLHTTP60,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS( &_req ) ) ) ){
		return;
	}

	Microsoft::WRL::Details::MakeAndInitialize<CXMLHTTPRequest2Callback>( &_cb );

	if( FAILED( _req->Open( req.ToCString<WCHAR>(),url.ToCString<WCHAR>(),_cb.Get(),0,0,0,0 ) ) ){
		Print( "Failed to open HttpRequest" );
	}
}

void BBHttpRequest::SetHeader( String name,String value ){

	_req->SetRequestHeader( name.ToCString<WCHAR>(),value.ToCString<WCHAR>() );
}

void BBHttpRequest::Send(){

	if( FAILED( _req->Send( 0,0 ) ) ){
		Print( "Send failed" );
	}
	
	Start();
}

void BBHttpRequest::SendText( String text,String encoding ){

	_data=Microsoft::WRL::Details::Make<CXMLHTTPRequestData>();
	
	int length=_data->SetText( text );
	
	if( FAILED( _req->Send( _data.Get(),length ) ) ){
		Print( "Send failed" );
	}

	Start();
}

void BBHttpRequest::Run__UNSAFE__(){

	Print( "Thread starting!" );

	_cb->Wait();
}

String BBHttpRequest::ResponseText(){
	return _cb ? _cb->ResponseText() : "";
}

int BBHttpRequest::Status(){
	return _cb ? _cb->Status() : -1;
}

int BBHttpRequest::BytesReceived(){
	return _cb ? _cb->BytesReceived() : 0;
}
