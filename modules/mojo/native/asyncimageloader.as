
class BBAsyncImageLoaderThread{

	internal var _running:Boolean=false;
	
	internal var _device:gxtkGraphics;
	internal var _path:String;
	internal var _surface:gxtkSurface;
	internal var _result:Boolean;

	public function Start():void{
		
		var thread:BBAsyncImageLoaderThread=this;
		
		thread._surface=null;
		thread._result=false;
		thread._running=true;
		
		var loader:Loader=new Loader();
		
		loader.contentLoaderInfo.addEventListener( Event.COMPLETE,onLoaded );
		loader.contentLoaderInfo.addEventListener( IOErrorEvent.IO_ERROR,onIoError );
		loader.contentLoaderInfo.addEventListener( SecurityErrorEvent.SECURITY_ERROR,onSecurityError );

		function onLoaded( e:Event ):void{
			thread._surface=new gxtkSurface( e.target.content );
			thread._result=true;
			thread._running=false;
		}
		
		function onIoError( e:IOErrorEvent ):void{
			thread._running=false;
		}

		function onSecurityError( e:SecurityErrorEvent ):void{
			thread._running=false;
		}
		
		loader.load( new URLRequest( BBGame.Game().PathToUrl( thread._path ) ) );
	}
	
	public function IsRunning():Boolean{
		return _running;
	}
}
