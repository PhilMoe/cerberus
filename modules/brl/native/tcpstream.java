
class BBTcpStream extends BBStream{
	
	java.net.Socket _sock;
	InputStream _input;
	OutputStream _output;
	int _state;				//0=INIT, 1=CONNECTED, 2=CLOSED, -1=ERROR

	boolean Connect( String addr,int port ){
	
		if( _state!=0 ) return false;
		
		try{
			_sock=new java.net.Socket( addr,port );
			if( _sock.isConnected() ){
				_input=_sock.getInputStream();
				_output=_sock.getOutputStream();
				_state=1;
				return true;
			}
		}catch( IOException ex ){
		}
		
		_state=1;
		_sock=null;
		return false;
	}
	
	int ReadAvail(){
		try{
			return _input.available();
		}catch( IOException ex ){
		}
		_state=-1;
		return 0;
	}
	
	int WriteAvail(){
		return 0;
	}
	
	int Eof(){
		if( _state>=0 ) return (_state==2) ? 1 : 0;
		return -1;
	}
	
	void Close(){

		if( _sock==null ) return;
		
		try{
			_sock.close();
			if( _state==1 ) _state=2;
		}catch( IOException ex ){
			_state=-1;
		}
		_sock=null;
	}
	
	int Read( BBDataBuffer buffer,int offset,int count ){

		if( _state!=1 ) return 0;
		
		try{
			int n=_input.read( buffer._data.array(),offset,count );
			if( n>=0 ) return n;
			_state=2;
		}catch( IOException ex ){
			_state=-1;
		}
		return 0;
	}
	
	int Write( BBDataBuffer buffer,int offset,int count ){

		if( _state!=1 ) return 0;
		
		try{
			_output.write( buffer._data.array(),offset,count );
			return count;
		}catch( IOException ex ){
			_state=-1;
		}
		return 0;
	}
}
