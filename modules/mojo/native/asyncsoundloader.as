
class BBAsyncSoundLoaderThread{

	internal var _running:Boolean=false;
	
	internal var _device:gxtkAudio;
	internal var _path:String;
	internal var _sample:gxtkSample;
	internal var _result:Boolean;

	public function Start():void{
		
		var thread:BBAsyncSoundLoaderThread=this;
		
		thread._sample=null;
		thread._result=false;
		thread._running=true;
		
		var sound:Sound=new Sound();
		
		sound.addEventListener( Event.COMPLETE,onLoaded );
		sound.addEventListener( IOErrorEvent.IO_ERROR,onIoError );
		sound.addEventListener( SecurityErrorEvent.SECURITY_ERROR,onSecurityError );
		
		function onLoaded( e:Event ):void{
			thread._sample=new gxtkSample( sound );
			thread._running=false;
		}
		
		function onIoError( e:IOErrorEvent ):void{
			thread._sample=null;
			thread._running=false;
		}

		function onSecurityError( e:SecurityErrorEvent ):void{
			thread._sample=null;
			thread._running=false;
		}
		
		sound.load( new URLRequest( BBGame.Game().PathToUrl( thread._path ) ) );
	}
	
	public function IsRunning():Boolean{
		return _running;
	}
}
