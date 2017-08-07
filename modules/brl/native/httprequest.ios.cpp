
// ***** HttpRequest.h *****

class BBHttpRequest : public BBThread{

	public:

	BBHttpRequest();
		
	void Open( String req,String url,int timeout,bool httpsVerifyCertificate, bool httpsVerifyHost );
	void SetHeader( String name,String value );
	void Send();
	void SendText( String text,String encoding );
	String ResponseText();
	int Status();
	int BytesReceived();
	
	private:

	NSMutableURLRequest *_req;
	String _response;
	int _status;
	int _recv;
	
	void Run__UNSAFE__();
};

// ***** HttpRequest.cpp *****

BBHttpRequest::BBHttpRequest():_req( 0 ),_status( -1 ),_recv( 0 ){
}

void BBHttpRequest::Open( String req,String url,int timeout,bool httpsVerifyCertificate, bool httpsVerifyHost ){
	
	_req=[[NSMutableURLRequest alloc] init];
	
	[_req setHTTPMethod:req.ToNSString()];
	[_req setURL:[NSURL URLWithString:url.ToNSString()]];
  [_req setTimeoutInterval:(timeout/1000)];

	if( [_req respondsToSelector:@selector(setAllowsCellularAccess:)] ){
		[_req setAllowsCellularAccess:YES];
	}
	
	_response="";
	_status=-1;
	_recv=0;
}

void BBHttpRequest::SetHeader( String name,String value ){

	[_req setValue:value.ToNSString() forHTTPHeaderField:name.ToNSString()];
}

void BBHttpRequest::Send(){

	Start();
}

void BBHttpRequest::SendText( String text,String encoding ){

	[_req setHTTPBody:[text.ToNSString() dataUsingEncoding:NSUTF8StringEncoding]];
	
	Start();
}

void BBHttpRequest::Run__UNSAFE__(){

	NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];

	NSURLResponse *response=0;
	
	NSData *data=[NSURLConnection sendSynchronousRequest:_req returningResponse:&response error:0];
	
	if( data && response ){
	
	    _response=String( [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] );

	    _status=[(NSHTTPURLResponse*)response statusCode];
	    
	    _recv=[data length];
	}
	
	[pool release];
	
	[_req release];
	
	_req=0;
}

String BBHttpRequest::ResponseText(){
	return _response;
}

int BBHttpRequest::Status(){
	return _status;
}

int BBHttpRequest::BytesReceived(){
	return _recv;
}
