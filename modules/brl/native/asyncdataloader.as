
class BBAsyncDataLoaderThread extends BBThread{

	internal var _buf:BBDataBuffer;
	internal var _path:String;
	internal var _result:Boolean;

	override public function Run__UNSAFE__():void{
		
		var thread:BBAsyncDataLoaderThread=this;
		
		var loader:URLLoader=new URLLoader();

		loader.dataFormat=URLLoaderDataFormat.BINARY;
		
		loader.addEventListener( Event.COMPLETE,onLoaded );
		loader.addEventListener( IOErrorEvent.IO_ERROR,onError );
		
		function onLoaded( e:Event ):void{
			thread._buf._Init( loader.data );
			thread._result=true;
			thread.running=false;
		}
		
		function onError( e:IOErrorEvent ):void{
			thread._result=false;
			thread.running=false;
		}
		
		loader.load( new URLRequest( BBGame.Game().PathToUrl( thread._path ) ) );
	}

}
