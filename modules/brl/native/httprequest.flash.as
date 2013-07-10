
class BBHttpRequest{

	internal var _req:URLRequest=null;
	internal var _loader:URLLoader=null;
	internal var _response:String="";
	internal var _status:int=-1;
	
	internal var _running:Boolean=false;

	public function Open( req:String,url:String ):void{
		_req=new URLRequest( url );
		_req.method=req;
		_response="";
		_status=-1;
	}
	
	public function SetHeader( name:String,value:String ):void{
		_req.requestHeaders.push( new URLRequestHeader( name,value ) );
	}
	
	public function Send():void{
		Start();
	}
	
	public function SendText( text:String,encoding:String ):void{
		_req.data=text;
		Start();
	}
	
	public function ResponseText():String{
		return _response;
	}
	
	public function Status():int{
		return _status;
	}
	
	public function BytesReceived():int{
		if( _loader ) return _loader.bytesLoaded;
		return 0;
	}
	
	public function IsRunning():Boolean{
		return _running;
	}
	
	internal function Start():void{
	
		var req:BBHttpRequest=this;
		
		_loader=new URLLoader( null );
		_loader.addEventListener( Event.COMPLETE,onComplete );
		_loader.addEventListener( IOErrorEvent.IO_ERROR,onError );
		_loader.addEventListener( HTTPStatusEvent.HTTP_STATUS,onStatus );

		var status:int=-1;
		
		function onComplete( e:Event ):void{
			req._response=_loader.data;
			req._status=status;
			req._running=false;
		}
		
		function onError( e:IOErrorEvent ):void{
			req._running=false;
		}

		function onStatus( e:HTTPStatusEvent ):void{
			status=e.status;
		}
		
		_running=true;
		_loader.load( _req );
	}
}