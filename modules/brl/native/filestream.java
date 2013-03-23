
class BBFileStream extends BBStream{

	boolean Open( String path,String mode ){
		if( _stream!=null ) return false;
		
		String fmode="";
		if( mode.equals("r") ){
			fmode="r";
		}else if( mode.equals("w") ){
			fmode="rw";
		}else if( mode.equals("u") ){
			fmode="rw";
		}else{
			return false;
		}
		
		try{
			_stream=BBGame.Game().OpenFile( path,fmode );
			if( _stream!=null ){
				if( mode.equals( "w" ) ) _stream.setLength( 0 );
				_position=_stream.getFilePointer();
				_length=_stream.length();
				return true;
			}
		}catch( IOException ex ){
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
		try{
			_stream.seek( offset );
			_position=_stream.getFilePointer();
		}catch( IOException ex ){
		}
		return (int)_position;
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
		if( _stream==null ) return 0;
		
		try{
			_stream.write( buffer._data.array(),offset,count );
			_position+=count;
			if( _position>_length ) _length=_position;
			return count;
		}catch( IOException ex ){
		}
		return 0;
	}

	RandomAccessFile _stream;
	long _position,_length;
}
