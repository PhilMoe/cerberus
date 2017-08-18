
import flash.display.*;
import flash.events.*;
import flash.media.*;
import flash.net.*;
import flash.utils.ByteArray;

import flash.ui.*;

class BBGameEvent{
	public static const KeyDown:int=1;
	public static const KeyUp:int=2;
	public static const KeyChar:int=3;
	public static const MouseDown:int=4;
	public static const MouseUp:int=5;
	public static const MouseMove:int=6;
	public static const TouchDown:int=7;
	public static const TouchUp:int=8;
	public static const TouchMove:int=9;
	public static const MotionAccel:int=10;
}

class BBGameDelegate{
	public function StartGame():void{}
	public function SuspendGame():void{}
	public function ResumeGame():void{}
	public function UpdateGame():void{}
	public function RenderGame():void{}
	public function KeyEvent( event:int,data:int ):void{}
	public function MouseEvent( event:int,data:int,x:Number,y:Number,z:Number ):void{}
	public function TouchEvent( event:int,data:int,x:Number,y:Number ):void{}
	public function MotionEvent( event:int,data:int,x:Number,y:Number,z:Number ):void{}
	public function DiscardGraphics():void{}
}

class BBDisplayMode{
	public var width:int;
	public var height:int;
}

class BBGame{

	internal static var _game:BBGame;

	internal var _delegate:BBGameDelegate;
	internal var _keyboardEnabled:Boolean;
	internal var _updateRate:int;
	internal var _debugExs:Boolean;
	internal var _started:Boolean;
	internal var _suspended:Boolean;
	internal var _startms:Number;
	
	public function BBGame(){
		_game=this;
		_debugExs=(Config.CONFIG=="debug");
		_startms=(new Date).getTime();
	}
	
	public static function Game():BBGame{
		return _game;
	}
	
	public function SetDelegate( delegate:BBGameDelegate ):void{
		_delegate=delegate;
	}
	
	public function Delegate():BBGameDelegate{
		return _delegate;
	}
	
	public function SetKeyboardEnabled( enabled:Boolean ):void{
		_keyboardEnabled=enabled;
	}
	
	public function SetUpdateRate( hertz:int ):void{
		_updateRate=hertz;
	}
	
	public function Started():Boolean{
		return _started;
	}
	
	public function Suspended():Boolean{
		return _suspended;
	}
	
	public function Millisecs():int{
		return (new Date).getTime()-_startms;
	}
	
	public function GetDate( date:Array ):void{
		var n:int=date.length;
		if( n>0 ){
			var t:Date=new Date();
			date[0]=t.getFullYear();
			if( n>1 ){
				date[1]=t.getMonth()+1;
				if( n>2 ){
					date[2]=t.getDate();
					if( n>3 ){
						date[3]=t.getHours();
						if( n>4 ){
							date[4]=t.getMinutes();
							if( n>5 ){
								date[5]=t.getSeconds();
								if( n>6 ){
									date[6]=t.getMilliseconds();
								}
							}
						}
					}
				}
			}
		}
	}
	
	public function SaveState( state:String ):int{
		var file:SharedObject=SharedObject.getLocal( "cerberusstate" );
		file.data.state=state;
		file.close();
		return 0;
	}
	
	public function LoadState():String{
		var file:SharedObject=SharedObject.getLocal( "cerberusstate" );
		var state:String=file.data.state;
		file.close();
		if( state ) return state;
		return "";
	}

	public function LoadString( path:String ):String{
		var buf:ByteArray=LoadData( path );
		if( buf ) return buf.toString();
		return "";
	}
	
	public function CountJoysticks( update:Boolean ):int{
		return 0;
	}
	
	public function PollJoystick( port:int,joyx:Array,joyy:Array,joyz:Array,buttons:Array ):Boolean{
		return false;
	}
	
	public function OpenUrl( url:String ):void{
		navigateToURL( new URLRequest( url ) );
	}
	
	public function SetMouseVisible( visible:Boolean ):void{
		if( visible ){
			Mouse.show();
		}else{
			Mouse.hide();
		}
	}
	
	public function GetDeviceWidth():int{
		return 0;
	}
	
	public function GetDeviceHeight():int{
		return 0;
	}
	
	public function SetDeviceWindow( width:int,height:int,flags:int ):void{
	}
	
	public function GetDisplayModes():Array{
		return new Array();
	}
	
	public function GetDesktopMode():BBDisplayMode{
		return null;
	}
	
	public function SetSwapInterval( interval:int ):void{
	}
	
	public function PathToFilePath( path:String ):String{
		return "";
	}
	
	//***** Flash Game *****
	
	public function PathToUrl( path:String ):String{
		return path;
	}
	
	public function LoadData( path:String ):ByteArray{
		//TODO: Load from URL
		return null;
	}
	
	//***** INTERNAL *****
	public function Die( ex:Object ):void{
	
		_delegate=new BBGameDelegate();
		
		if( !ex.toString() ){
			return;
		}
		if( _debugExs ){
			print( "Cerberus Runtime Error : "+ex.toString() );
			print( stackTrace() );
		}
		throw ex;
	}
	
	public function StartGame():void{
	
		if( _started ) return;
		_started=true;
		
		try{
			_delegate.StartGame();
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function SuspendGame():void{
	
		if( !_started || _suspended ) return;
		_suspended=true;
		
		try{
			_delegate.SuspendGame();
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function ResumeGame():void{

		if( !_started || !_suspended ) return;
		_suspended=false;
		
		try{
			_delegate.ResumeGame();
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function UpdateGame():void{

		if( !_started || _suspended ) return;
		
		try{
			_delegate.UpdateGame();
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function RenderGame():void{

		if( !_started ) return;
		
		try{
			_delegate.RenderGame();
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function KeyEvent( ev:int,data:int ):void{

		if( !_started ) return;
		
		try{
			_delegate.KeyEvent( ev,data );
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function MouseEvent( ev:int,data:int,x:Number,y:Number,z:Number ):void{

		if( !_started ) return;
		
		try{
			_delegate.MouseEvent( ev,data,x,y,z );
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function TouchEvent( ev:int,data:int,x:Number,y:Number ):void{

		if( !_started ) return;
		
		try{
			_delegate.TouchEvent( ev,data,x,y );
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function MotionEvent( ev:int,data:int,x:Number,y:Number,z:Number ):void{

		if( !_started ) return;
		
		try{
			_delegate.MotionEvent( ev,data,x,y,z );
		}catch( ex:Object ){
			Die( ex );
		}
	}
	
	public function DiscardGraphics():void{

		if( !_started ) return;
		
		try{
			_delegate.DiscardGraphics();
		}catch( ex:Object ){
			Die( ex );
		}
	}
}
