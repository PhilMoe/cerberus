
class BBThread{

	internal var result:Object=null;
	internal var running:Boolean=false;
	
	public function Start():void{
		result=null;
		running=true;
		Run__UNSAFE__();
	}
	
	public function IsRunning():Boolean{
		return running;
	}
	
	public function Result():Object{
		return result;
	}
	
	public function Run__UNSAFE__():void{
		running=false;
	}
}
