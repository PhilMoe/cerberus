
public class BBDataBuffer{

    public byte[] _data;
    public int _length;
    
    public BBDataBuffer(){
    }

    public BBDataBuffer( int length ){
    	_data=new byte[length];
    	_length=length;
    }
    
    public BBDataBuffer( byte[] data ){
    	_data=data;
    	_length=data.Length;
    }
    
    public virtual bool _New( int length ){
    	if( _data!=null ) return false;
    	_data=new byte[length];
    	_length=length;
    	return true;
    }
    
    public virtual bool _Load( String path ){
    	if( _data!=null ) return false;
    	
    	_data=BBGame.Game().LoadData( path );
    	if( _data==null ) return false;
    	
    	_length=_data.Length;
    	return true;
    }
    
    public virtual void _LoadAsync( String path,BBThread thread ){
    	if( _Load( path ) ) thread.SetResult( this );
    }

    public virtual byte[] GetByteArray(){
    	return _data;
    }
    
    public virtual void Discard(){
    	if( _data!=null ){
    		_data=null;
    		_length=0;
    	}
    }
    
  	public virtual int Length(){
  		return _length;
  	}

	public virtual void PokeByte( int addr,int value ){
		_data[addr]=(byte)value;
	}

	public virtual void PokeShort( int addr,int value ){
		Array.Copy( System.BitConverter.GetBytes( (short)value ),0,_data,addr,2 );
	}

	public virtual void PokeInt( int addr,int value ){
		Array.Copy( System.BitConverter.GetBytes( value ),0,_data,addr,4 );
	}

	public virtual void PokeFloat( int addr,float value ){
		Array.Copy( System.BitConverter.GetBytes( value ),0,_data,addr,4 );
	}

	public virtual int PeekByte( int addr ){
		return (int)(sbyte)_data[addr];
	}

	public virtual int PeekShort( int addr ){
		return (int)System.BitConverter.ToInt16( _data,addr );
	}

	public virtual int PeekInt( int addr ){
		return System.BitConverter.ToInt32( _data,addr );
	}

	public virtual float PeekFloat( int addr ){
		return (float)System.BitConverter.ToSingle( _data,addr );
	}
}