
class BBFileInputStream extends BBStream{

	boolean Open( String path ){
		if( _stream!=null ) return false;
						
		try{
			_stream=BBGame.Game().OpenInputStream(path);
			if( _stream!=null ){
				_position=0;
				_length=_stream.available();
				return true;
			}
		
		}catch( IOException ex ){
			throw new RuntimeException(ex);
		}
		
		_stream=null;
		_position=0;
		_length=0;
		return false;
	}

	void Close(){
		if( _stream==null ) return;

		try{
			_stream.close();
		}catch( IOException ex ){
		}
		_stream=null;
		_position=0;
		_length=0;
	}
	
	int Eof(){
		if( _stream==null ) return -1;
		
		return (_position==_length) ? 1 : 0;
	}
	
	int Length(){
		return (int)_length;
	}
	
	int Offset(){
		return (int)_position;
	}
	
	int Seek( int offset ){
		return 0;
	}
		
	int Read( BBDataBuffer buffer,int offset,int count ){
		if( _stream==null ) return 0;
		
		try{
			int n=_stream.read( buffer._data.array(),offset,count );
			if( n>=0 ){
				_position+=n;
				return n;
			}
		}catch( IOException ex ){
		}
		return 0;
	}
	
	int Write( BBDataBuffer buffer,int offset,int count ){
		return 0;
	}

	InputStream _stream;
	long _position,_length;
}
