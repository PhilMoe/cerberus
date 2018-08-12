
class BBFlashGame extends BBGame{

	internal static var _flashGame:BBFlashGame;
	
	internal var _root:DisplayObjectContainer;
	
	internal var _nextUpdate:Number;
	internal var _updatePeriod:Number;

	public function BBFlashGame( root:DisplayObjectContainer ){
		_flashGame=this;
		_root=root;
	}
	
	public static function FlashGame():BBFlashGame{
		return _flashGame;
	}
	
	internal function KeyToChar( key:int ):int{
		switch( key ){
		case 8:case 9:case 13:case 27:return key;
		case 33:case 34:case 35:case 36:case 37:case 38:case 39:case 40:case 45:return key | 0x10000;
		case 46:return 127;
		}
		return 0;
	}
	
	internal function ValidateUpdateTimer():void{
		if( _suspended ){
			_root.stage.frameRate=24;
		}else if( _updateRate ){
			_updatePeriod=1000.0/_updateRate;
			_nextUpdate=0;
			_root.stage.frameRate=_updateRate;
		}else{
			_root.stage.frameRate=60;
		}
	}
	
	//***** BBGame *****	
	
	public override function GetDeviceWidth():int{
		return _root.stage.stageWidth;
	}
	
	public override function GetDeviceHeight():int{
		return _root.stage.stageHeight;
	}
	
	public override function SetUpdateRate( hertz:int ):void{
		super.SetUpdateRate( hertz );
		ValidateUpdateTimer();
	}
	
	public override function PathToUrl( path:String ):String {
		if( path.indexOf( "cerberus:" )!=0 ){
			return path;
		}else if( path.indexOf( "cerberus://data/" )==0 ){
			return "data/"+path.slice( 16 );
		}
		return "";
	}

	public override function LoadData( path:String ):ByteArray{
		var t:Class=GetAsset( path );
		if( t ) return (new t) as ByteArray;
		return null;
	}

	public function GetDisplayObjectContainer():DisplayObjectContainer{
		return _root;
	}

	public function GetAsset( path:String ):Class{
		if( path.indexOf( "cerberus://data/" )!=0 ) return null;

		path=path.slice(16);
		
		var i:int=path.indexOf( "." ),ext:String="";
		if( i!=-1 ){
			ext=path.slice(i+1);
			path=path.slice(0,i);
		}

		var munged:String="_";
		var bits:Array=path.split( "/" );
		
		for( i=0;i<bits.length;++i ){
			munged+=bits[i].length+bits[i];
		}
		munged+=ext.length+ext;
		
		return Assets[munged];
	}
	
	public function LoadBitmap( path:String ):Bitmap{
		var t:Class=GetAsset( path );
		if( t ) return (new t) as Bitmap;
		return null;
	}
	
	public function LoadSound( path:String ):Sound{
		var t:Class=GetAsset( path );
		if( t ) return (new t) as Sound;
		return null;
	}
	
	//***** INTERNAL *****
	
	public override function SuspendGame():void{
		super.SuspendGame();
		super.RenderGame();
		ValidateUpdateTimer();
	}
	
	public override function ResumeGame():void{
		super.ResumeGame();
		ValidateUpdateTimer();
	}

	public function Run():void{

		_root.stage.addEventListener( Event.ACTIVATE,OnActivate );
		_root.stage.addEventListener( Event.DEACTIVATE,OnDeactivate );
		_root.stage.addEventListener( Event.ENTER_FRAME,OnEnterFrame );
		_root.stage.addEventListener( KeyboardEvent.KEY_DOWN,OnKeyDown );
		_root.stage.addEventListener( KeyboardEvent.KEY_UP,OnKeyUp );
		_root.stage.addEventListener( flash.events.MouseEvent.MOUSE_DOWN,OnMouseDown );
		_root.stage.addEventListener( flash.events.MouseEvent.MOUSE_UP,OnMouseUp );
		_root.stage.addEventListener( flash.events.MouseEvent.MOUSE_MOVE,OnMouseMove );
		
		StartGame();
	}

	public function OnActivate( e:Event ):void{
		if( Config.MOJO_AUTO_SUSPEND_ENABLED=="1" ) ResumeGame();
	}
	
	public function OnDeactivate( e:Event ):void{
		if( Config.MOJO_AUTO_SUSPEND_ENABLED=="1" ) SuspendGame();
	}
	
	public function OnEnterFrame( e:Event ):void{
		if( _suspended ){
			if( Config.FLASH_RENDER_WHILE_SUSPENDED=="1" ) RenderGame();
			return;
		}
		
		if( !_updateRate ){
			UpdateGame();
			RenderGame();
			return;
		}
		
		if( !_nextUpdate ) _nextUpdate=(new Date).getTime();
		
		var i:int;
		for( i=0;i<4;++i ){
		
			UpdateGame();
			if( !_nextUpdate ) break;
			
			_nextUpdate+=_updatePeriod;
			if( (new Date).getTime()<_nextUpdate ) break;
		}
		if( i==4 ) _nextUpdate=0;
		RenderGame();
	}
	
	public function OnKeyDown( e:KeyboardEvent ):void{
		KeyEvent( BBGameEvent.KeyDown,e.keyCode );
		if( e.charCode!=0 ){
			KeyEvent( BBGameEvent.KeyChar,e.charCode );
		}else{
			var chr:int=KeyToChar( e.keyCode );
			if( chr ) KeyEvent( BBGameEvent.KeyChar,chr );
		}
	}

	public function OnKeyUp( e:KeyboardEvent ):void{
		KeyEvent( BBGameEvent.KeyUp,e.keyCode );
	}
		
	public function OnMouseDown( e:flash.events.MouseEvent ):void{
		MouseEvent( BBGameEvent.MouseDown,0,e.localX,e.localY,0 );
	}
		
	public function OnMouseUp( e:flash.events.MouseEvent ):void{
		MouseEvent( BBGameEvent.MouseUp,0,e.localX,e.localY,0 );
	}

	public function OnMouseMove( e:flash.events.MouseEvent ):void{
		MouseEvent( BBGameEvent.MouseMove,-1,e.localX,e.localY,0 );
	}

}
