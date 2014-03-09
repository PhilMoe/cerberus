
function BBAsyncSoundLoaderThread(){
	this._running=false;
}
  
BBAsyncSoundLoaderThread.prototype.Start=function(){

	this._sample=null;
	if( !this._device.okay ) return;
	
	var audio=new Audio();
	if( !audio ) return;
	
	var thread=this;
	thread._running=true;

	audio.src=BBGame.Game().PathToUrl( this._path );
	audio.preload='auto';	
	
	var success=function( e ){
		thread._sample = new gxtkSample( audio );
		thread._running=false;
		audio.removeEventListener( 'canplaythrough',success,false );
		audio.removeEventListener( 'error',error,false );
	}
	
	var error=function( e ){
		thread._running=false;
		audio.removeEventListener( 'canplaythrough',success,false );
		audio.removeEventListener( 'error',error,false );
	}
	
	audio.addEventListener( 'canplaythrough',success,false );
	audio.addEventListener( 'error',error,false );
	
	audio.load();
  }
  
BBAsyncSoundLoaderThread.prototype.IsRunning=function(){
	return this._running;
}
