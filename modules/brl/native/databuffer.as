
class BBDataBuffer{

	internal var _data:ByteArray=null;
	internal var _length:int=0;
	
	public function _Init( data:ByteArray ):void{
		_data=data;
		_length=data.length;
	}
	
	public function _New( length:int ):Boolean{
		if( _data ) return false
		_data=new ByteArray;
		_data.length=length;
		_length=length;
		return true;
	}
	
	public function _Load( path:String ):Boolean{
		if( _data ) return false
		var data:ByteArray=BBGame.Game().LoadData( path );
		if( !data ) return false;
		_Init( data );
		return true;
	}
	
	public function _LoadAsync( path:String,thread:BBThread ):void{

		var buf:BBDataBuffer=this;
		
		var loader:URLLoader=new URLLoader();
		loader.dataFormat=URLLoaderDataFormat.BINARY;
		loader.addEventListener( Event.COMPLETE,onLoaded );
		loader.addEventListener( IOErrorEvent.IO_ERROR,onError );
		
		function onLoaded( e:Event ):void{
			buf._Init( loader.data );
			thread.result=buf;
			thread.running=false;
		}
		
		function onError( e:IOErrorEvent ):void{
			thread.running=false;
		}
		
		loader.load( new URLRequest( BBGame.Game().PathToUrl( path ) ) );
	}

	public function GetByteArray():ByteArray{
		return _data;
	}
	
	public function Discard():void{
		if( _data ){
			_data.clear();
			_data=null;
			_length=0;
		}
	}
	
	public function Length():int{
		return _length;
	}
	
	public function PokeByte( addr:int,value:int ):void{
		_data.position=addr;
		_data.writeByte( value );
	}
	
	public function PokeShort( addr:int,value:int ):void{
		_data.position=addr;
		_data.writeShort( value );
	}
	
	public function PokeInt( addr:int,value:int ):void{
		_data.position=addr;
		_data.writeInt( value );
	}
	
	public function PokeFloat( addr:int,value:Number ):void{
		_data.position=addr;
		_data.writeFloat( value );
	}
	
	public function PeekByte( addr:int ):int{
		_data.position=addr;
		return _data.readByte();
	}
	
	public function PeekShort( addr:int ):int{
		_data.position=addr;
		return _data.readShort();
	}

	public function PeekInt( addr:int ):int{
		_data.position=addr;
		return _data.readInt();
	}
	
	public function PeekFloat( addr:int ):Number{
		_data.position=addr;
		return _data.readFloat();
	}
}
