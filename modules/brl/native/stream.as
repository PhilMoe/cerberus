
class BBStream{

	public function Eof():int{
		return 0;
	}
	
	public function Close():void{
	}
	
	public function Length():int{
		return 0;
	}
	
	public function Position():int{
		return 0;
	}
	
	public function Seek( position:int ):int{
		return 0;
	}
	
	public function Read( buffer:BBDataBuffer,offset:int,count:int ):int{
		return 0;
	}
	
	public function Write( buffer:BBDataBuffer,offset:int,count:int ):int{
		return 0;
	}
}
