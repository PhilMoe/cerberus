
class BBDataBuffer{
	
	ByteBuffer _data;
	int _length;

	boolean _New( int length ){
		if( _data!=null ) return false;
		_data=ByteBuffer.allocate( length );
		_data.order( ByteOrder.nativeOrder() );
		_length=length;
		return true;
	}
	
	boolean _New( int length,boolean direct ){
		if( _data!=null ) return false;
		if( direct ){
			_data=ByteBuffer.allocateDirect( length );
		}else{
			_data=ByteBuffer.allocate( length );
		}
		_data.order( ByteOrder.nativeOrder() );
		_length=length;
		return true;
	}
	
	boolean _Load( String path ){
		if( _data!=null ) return false;
	
		byte[] data=BBGame.Game().LoadData( path );
		if( data==null ) return false;
		
		if( !_New( data.length ) ) return false;
		
		System.arraycopy( data,0,_data.array(),0,data.length );
		return true;
	}
	
	void _LoadAsync( String path,BBThread thread ){
		if( _Load( path ) ) thread.SetResult( this );
	}

	ByteBuffer GetByteBuffer(){
		return _data;
	}
	
	int Length(){
		return _length;
	}
	
	void Discard(){
		if( _data==null ) return;
		_data=null;
		_length=0;
	}
		
	void PokeByte( int addr,int value ){
		_data.put( addr,(byte)value );
	}
	
	void PokeShort( int addr,int value ){
		_data.putShort( addr,(short)value );
	}
	
	void PokeInt( int addr,int value ){
		_data.putInt( addr,value );
	}
	
	void PokeFloat( int addr,float value ){
		_data.putFloat( addr,value );
	}
	
	int PeekByte( int addr ){
		return _data.get( addr );
	}
	
	int PeekShort( int addr ){
		return _data.getShort( addr );
	}
	
	int PeekInt( int addr ){
		return _data.getInt( addr );
	}
	
	float PeekFloat( int addr ){
		return _data.getFloat( addr );
	}
}
