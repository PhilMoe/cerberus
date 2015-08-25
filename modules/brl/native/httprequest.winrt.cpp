
// ***** HttpRequest.h *****

#include <msxml6.h>
#include <wrl/client.h>
#include <wrl/implements.h>

class BBHttpRequest;

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
	
	void Start();
	void Failed();
	
	String ResponseText(){ return _response; }
	int Status(){ return _status; }
	int BytesReceived(){ return _recv; }
	bool IsRunning(){ return _running; }
	
private:

    friend HRESULT Microsoft::WRL::Details::MakeAndInitialize<CXMLHTTPRequest2Callback,CXMLHTTPRequest2Callback>( CXMLHTTPRequest2Callback** );

	String _response;
	int _status;
	int _recv;
	bool _running;
	
	CXMLHTTPRequest2Callback();
	~CXMLHTTPRequest2Callback();

    STDMETHODIMP RuntimeClassInitialize();
};

class BBHttpRequest : public Object{

public:

	BBHttpRequest();
		
	void Open( String req,String url );
	void SetHeader( String name,String value );
	void Send();
	void SendText( String text,String encoding );
	
	String ResponseText(){ return _cb->ResponseText(); }
	int Status(){ return _cb->Status(); }
	int BytesReceived(){ return _cb->BytesReceived(); }
	bool IsRunning(){ return _cb->IsRunning(); }
	
private:
	
	Microsoft::WRL::ComPtr<IXMLHTTPRequest2> _req;
	Microsoft::WRL::ComPtr<CXMLHTTPRequestData> _data;
	Microsoft::WRL::ComPtr<CXMLHTTPRequest2Callback> _cb;
};

// ***** XMLHttpRequestData.cpp *****

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


// ***** XMLHttpRequest2Callback.cpp *****

CXMLHTTPRequest2Callback::CXMLHTTPRequest2Callback():_response( "" ),_status( -1 ),_recv( 0 ),_running( false ){
}

CXMLHTTPRequest2Callback::~CXMLHTTPRequest2Callback(){
}

STDMETHODIMP CXMLHTTPRequest2Callback::RuntimeClassInitialize(){
//	bbPrint( "RuntimeClassInitialize" );

	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnDataAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_opt ISequentialStream *pResponseStream ){
//	bbPrint( "Data" );

	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnError( __RPC__in_opt IXMLHTTPRequest2 *pXHR,HRESULT hrError ){
//	bbPrint( "Error" );
	
	_status=-1;
	
	_running=false;

	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnHeadersAvailable( __RPC__in_opt IXMLHTTPRequest2 *pXHR,DWORD dwStatus,__RPC__in_string const WCHAR *pwszStatus ){
//	bbPrint( "Headers" );
	
	_status=dwStatus;

	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnRedirect( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_string const WCHAR *pwszRedirectUrl ){
//	bbPrint( "Redirect" );
	
	return 0;
}

STDMETHODIMP CXMLHTTPRequest2Callback::OnResponseReceived( __RPC__in_opt IXMLHTTPRequest2 *pXHR,__RPC__in_opt ISequentialStream *pStream ){
//	bbPrint( "Response" );

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
	
	_running=false;
	
	return 0;
}

void CXMLHTTPRequest2Callback::Start(){

	_running=true;
}

void CXMLHTTPRequest2Callback::Failed(){

	_running=false;
}

// ***** HttpRequest.cpp *****

BBHttpRequest::BBHttpRequest(){
	Microsoft::WRL::Details::MakeAndInitialize<CXMLHTTPRequest2Callback>( &_cb );
}

void BBHttpRequest::Open( String req,String url ){
	
	if( FAILED( CoCreateInstance( CLSID_FreeThreadedXMLHTTP60,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS( &_req ) ) ) ){
		return;
	}

	if( FAILED( _req->Open( req.ToCString<WCHAR>(),url.ToCString<WCHAR>(),_cb.Get(),0,0,0,0 ) ) ){
	//	bbPrint( "Failed to open HttpRequest" );
	}
}

void BBHttpRequest::SetHeader( String name,String value ){

	_req->SetRequestHeader( name.ToCString<WCHAR>(),value.ToCString<WCHAR>() );
}

void BBHttpRequest::Send(){
	
	_cb->Start();

	if( FAILED( _req->Send( 0,0 ) ) ){
	//	bbPrint( "Send failed" );
		_cb->Failed();
	}
}

void BBHttpRequest::SendText( String text,String encoding ){

	_data=Microsoft::WRL::Details::Make<CXMLHTTPRequestData>();
	
	int length=_data->SetText( text );

	_cb->Start();
	
	if( FAILED( _req->Send( _data.Get(),length ) ) ){
	//	bbPrint( "Send failed" );
		_cb->Failed();
	}
}
