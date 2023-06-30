
class BBFileInputStream extends BBStream{

	boolean Open( String path ){
		if( _stream!=null ) return false;
				
		//try{
			_stream=BBGame.Game().OpenInputStream(path);
			if( _stream!=null ){
				_position=0; //_stream.getFilePointer();
				_length=100; //_stream.length();
				return true;
			}
		
		//}catch( IOException ex ){
		//}
		
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
		/*
		try{
			_stream.seek( offset );
			_position=_stream.getFilePointer();
		}catch( IOException ex ){
		}
		return (int)_position;
		*/
		return 1;
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
		/*
		if( _stream==null ) return 0;
		
		try{
			_stream.write( buffer._data.array(),offset,count );
			_position+=count;
			if( _position>_length ) _length=_position;
			return count;
		}catch( IOException ex ){
		}
		return 0;
		*/
		return 0;
	}

	InputStream _stream;
	long _position,_length;
}
