
class FileStream extends Stream{

	boolean Open( String path,String mode ){
	
		if( _file!=null ) return false;
	
		try{
			_file=new RandomAccessFile( path,mode );
		}catch( FileNotFoundException ex ){
			return false;
		}
		_off=_file.getFilePointer();
		_len=_file.length();
	}

	int Length(){
		return (int)_len;
	}
	
	int Offset(){
		return (int)_off;
	}
	
	int Seek( int offset ){
		_file.seek( offset );
		_off=_file.getFilePointer();
	
	}
		
	int Eof(){
		return (_off==_len) ? 1 : 0;
	}
	
	void Close(){
		if( _file==null ) return;
		_file=null;
		_off=0;
		_len=0;
	}
	
	int SkipBytes( int count ){
	
		if( _file==null ) return 0;
		
		return _file.skipBytes( count );
	}
	
	int ReadBytes( DataBuffer buffer,int count,int offset ){
	
		if( _file==null ) return 0;
		
		ByteBuffer buf=buffer.GetBuffer();
		
		if( buf.isDirect() ){
			byte[] arr=new byte[count];
			n=_file.read( arr,0,count );
			if( n<=0 ) return 0;
			buf.mark();
			buf.position( offset );
			buf.put( arr,0,n );
			buf.reset();
			return n;
		}else{
			int n=_file.read( buf.array(),buf.offset()+offset,count );
			return n>0 ? n : 0;
		}
	}
	
	int WriteBytes( DataBuffer buffer,int count,int offset ){
	
		if( _file==null ) return 0;
		
		ByteBuffer buf=buffer.GetByteBuffer();
		
		if( buf.isDirect() ){
			byte[] arr=new byte[count];
			buf.mark();
			buf.position( offset );
			buf.get( arr,0,count );
			buf.reset();
			int n=_file.write( arr,0,count );
			return n>0 ? n : 0;
		}else{
			int n=_file.write( buf.array(),buf.offset()+offset,count );
			return n>0 ? n : 0;
		}
	}

	RandromAccessFile _file;
	long _off,_len;
}
