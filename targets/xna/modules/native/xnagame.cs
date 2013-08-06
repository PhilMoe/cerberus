
public class BBXnaDisplayMode{

	public int Width;
	public int Height;
	public int Format;
		
	public BBXnaDisplayMode( int width,int height,int format ){
		Width=width;
		Height=height;
		Format=format;
	}
};

public class BBXnaGame : BBGame{

	static BBXnaGame _xnaGame;
	
	GraphicsDeviceManager _devman;

	Game _app;
	bool _exit;
		
#if WINDOWS
	System.Windows.Forms.Form _form;
#endif	
	
	bool _activated;
	bool _autoSuspend;
	bool _drawSuspended;
	
	double _nextUpdate;
	double _updatePeriod;
	
	bool _shift,_control;
	KeyboardState _keyboard;
		
	MouseState _mouse;
	int[] _touches=new int[32];

#if WINDOWS_PHONE
	bool _gamePadFound=true;	//for back button!
#else		
	bool _gamePadFound=false;
#endif

#if WINDOWS_PHONE
	Accelerometer _accelerometer;
#endif

	PlayerIndex _gamePadIndex=PlayerIndex.One;
	
	DisplayMode _desktop;
	
	public const int KEY_SHIFT=0x10;
	public const int KEY_CONTROL=0x11;
	public const int KEY_JOY0_A=0x100;
	public const int KEY_JOY0_B=0x101;
	public const int KEY_JOY0_X=0x102;
	public const int KEY_JOY0_Y=0x103;
	public const int KEY_JOY0_LB=0x104;
	public const int KEY_JOY0_RB=0x105;
	public const int KEY_JOY0_BACK=0x106;
	public const int KEY_JOY0_START=0x107;
	public const int KEY_JOY0_LEFT=0x108;
	public const int KEY_JOY0_UP=0x109;
	public const int KEY_JOY0_RIGHT=0x10a;
	public const int KEY_JOY0_DOWN=0x10b;
	
	public BBXnaGame( Game app ){
		_app=app;
		_xnaGame=this;
		
		_desktop=GraphicsAdapter.DefaultAdapter.CurrentDisplayMode;
		
		_devman=new GraphicsDeviceManager( _app );
		_devman.PreparingDeviceSettings+=new EventHandler<PreparingDeviceSettingsEventArgs>( PreparingDeviceSettings );
		
#if WINDOWS
		_devman.IsFullScreen=MonkeyConfig.XNA_WINDOW_FULLSCREEN=="1";
		_devman.PreferredBackBufferWidth=int.Parse( MonkeyConfig.XNA_WINDOW_WIDTH );
		_devman.PreferredBackBufferHeight=int.Parse( MonkeyConfig.XNA_WINDOW_HEIGHT );
		_app.Window.AllowUserResizing=MonkeyConfig.XNA_WINDOW_RESIZABLE=="1";
#elif XBOX
		_devman.IsFullScreen=MonkeyConfig.XNA_WINDOW_FULLSCREEN_XBOX=="1";
		_devman.PreferredBackBufferWidth=int.Parse( MonkeyConfig.XNA_WINDOW_WIDTH_XBOX );
		_devman.PreferredBackBufferHeight=int.Parse( MonkeyConfig.XNA_WINDOW_HEIGHT_XBOX );
#elif WINDOWS_PHONE
		_devman.IsFullScreen=MonkeyConfig.XNA_WINDOW_FULLSCREEN_PHONE=="1";
		_devman.PreferredBackBufferWidth=int.Parse( MonkeyConfig.XNA_WINDOW_WIDTH_PHONE );
		_devman.PreferredBackBufferHeight=int.Parse( MonkeyConfig.XNA_WINDOW_HEIGHT_PHONE );
#endif
	}
	
	public static BBXnaGame XnaGame(){
		return _xnaGame;
	}
	
	void PreparingDeviceSettings( Object sender,PreparingDeviceSettingsEventArgs e ){
		if( MonkeyConfig.XNA_VSYNC_ENABLED=="1" ){
			PresentationParameters pp=e.GraphicsDeviceInformation.PresentationParameters;
			pp.PresentationInterval=PresentInterval.One;
		}
	}
	
	public BBXnaDisplayMode GetXnaDesktopMode(){
		return new BBXnaDisplayMode( _desktop.Width,_desktop.Height,1 );
	}
	
	public BBXnaDisplayMode[] GetXnaDisplayModes(){
		List<BBXnaDisplayMode> modes=new List<BBXnaDisplayMode>();
		foreach( DisplayMode mode in GraphicsAdapter.DefaultAdapter.SupportedDisplayModes ){
			if( mode.Format==SurfaceFormat.Color ){
				modes.Add( new BBXnaDisplayMode( mode.Width,mode.Height,1 ) );
			}
		}
		return modes.ToArray();
	}
	
	public void SetXnaDisplayMode( int width,int height,int format,bool fullscreen ){
		_devman.IsFullScreen=fullscreen;
		_devman.PreferredBackBufferWidth=width;
		_devman.PreferredBackBufferHeight=height;
		_devman.ApplyChanges();
	}
	
	int KeyToChar( int key ){
		if( key>=48 && key<=57 && !_shift ) return key;
		if( key>=65 && key<=90 &&  _shift ) return key;
		if( key>=65 && key<=90 && !_shift ) return key+32;
	 	if( key==8 || key==9 || key==27 || key==32 ) return key;
		if( key>=33 && key<=40 || key==45 ) return key | 0x10000;
	 	if( key==46 ) return 127;
		return 0;
	}
	
	bool PollSuspended(){
	
		//wait for first activation		
		if( !_activated ){
			_activated=_app.IsActive;
			return true;
		}
		
		bool suspended;
		
		if( _autoSuspend ){
#if WINDOWS
			if( _form!=null ){
				suspended=!_form.Focused || _form.WindowState==System.Windows.Forms.FormWindowState.Minimized;
			}else{
				suspended=!_app.IsActive;
			}
#else		
			suspended=!_app.IsActive;
#endif
		}else{
			suspended=!(_app.Window.ClientBounds.Width>0 && _app.Window.ClientBounds.Height>0);
		}
		
		if( suspended!=_suspended ){
			if( suspended ){
				SuspendGame();
#if WINDOWS
				_drawSuspended=!_devman.IsFullScreen;
#endif
			}else{
				ResumeGame();
			}
		}
		return _suspended;
	}
	
	void ValidateUpdateTimer(){
		if( _updateRate!=0 && !_suspended ){
			_updatePeriod=1000.0/(double)_updateRate;
			_nextUpdate=(double)Millisecs()+_updatePeriod;
			_app.TargetElapsedTime=TimeSpan.FromTicks( (long)(10000000.0/(double)_updateRate+.5) );
			_app.IsFixedTimeStep=(MonkeyConfig.XNA_VSYNC_ENABLED!="1");
		}else{
			_app.TargetElapsedTime=TimeSpan.FromSeconds( 1.0/10.0 );
			_app.IsFixedTimeStep=true;
		}
	}
	
	void PollKeyboard(){
#if WINDOWS	
		KeyboardState kb=Keyboard.GetState();
		
		if( (kb.IsKeyDown( Keys.LeftShift ) || kb.IsKeyDown( Keys.RightShift ))!=_shift ){
			_shift=!_shift;
			KeyEvent( _shift ? BBGameEvent.KeyDown : BBGameEvent.KeyUp,KEY_SHIFT );
		}
		
		if( (kb.IsKeyDown( Keys.LeftControl ) || kb.IsKeyDown( Keys.RightControl ))!=_control ){
			_control=!_control;
			KeyEvent( _control ? BBGameEvent.KeyDown : BBGameEvent.KeyUp,KEY_CONTROL );
		}
		
		for( int i=8;i<256;++i ){
			if( i==KEY_SHIFT || i==KEY_CONTROL ) continue;
			
			if( kb.IsKeyDown( (Keys)i ) ){
				if( _keyboard.IsKeyDown( (Keys)i ) ) continue;
				
				KeyEvent( BBGameEvent.KeyDown,i );
				
				int chr=KeyToChar( i );
				
				if( chr!=0 ) KeyEvent( BBGameEvent.KeyChar,chr );
				
			}else{
				if( !_keyboard.IsKeyDown( (Keys)i ) ) continue;

				KeyEvent( BBGameEvent.KeyUp,i );
			}
		}
		
		_keyboard=kb;
#endif		
	}

	void PollMouse(){
#if WINDOWS
		MouseState m=Mouse.GetState();
	
		int ev=BBGameEvent.None;
		int data=-1;
		
		if( m.LeftButton!=_mouse.LeftButton ){
			ev=m.LeftButton==ButtonState.Pressed ? BBGameEvent.MouseDown : BBGameEvent.MouseUp;data=0;
		}
		if( m.RightButton!=_mouse.RightButton ){
			ev=m.RightButton==ButtonState.Pressed ? BBGameEvent.MouseDown : BBGameEvent.MouseUp;data=1;
		}
		if( m.MiddleButton!=_mouse.MiddleButton ){
			ev=m.MiddleButton==ButtonState.Pressed ? BBGameEvent.MouseDown : BBGameEvent.MouseUp;data=2;
		}
		
		if( ev!=BBGameEvent.None ){
			MouseEvent( ev,data,m.X,m.Y );
		}else if( m.X!=_mouse.X || m.Y!=_mouse.Y ){
			MouseEvent( BBGameEvent.MouseMove,-1,m.X,m.Y );
		}
		_mouse=m;
#endif		
	}
	
	void PollTouch(){
#if WINDOWS_PHONE
		TouchCollection tc=TouchPanel.GetState();
		foreach( TouchLocation tl in tc ){
			if( tl.State==TouchLocationState.Invalid ) continue;
		
			int touch=tl.Id;
			
			int pid;
			for( pid=0;pid<32 && _touches[pid]!=touch;++pid ){}
			
			int ev=BBGameEvent.None;

			switch( tl.State ){
			case TouchLocationState.Pressed:
				if( pid!=32 ) break;
				for( pid=0;pid<32 && _touches[pid]!=0;++pid ){}
				if( pid==32 ) break;
				_touches[pid]=touch;
				ev=BBGameEvent.TouchDown;
				break;
			case TouchLocationState.Released:
				if( pid==32 ) break;
				_touches[pid]=0;
				ev=BBGameEvent.TouchUp;
				break;
			case TouchLocationState.Moved:
				ev=BBGameEvent.TouchMove;
				break;
			}
			
			if( ev==BBGameEvent.None ) continue;
			
			TouchEvent( ev,pid,tl.Position.X,tl.Position.Y );
		}
#endif			
	}

#if WINDOWS_PHONE
	void OnAccelerometerReadingChanged( object sender,AccelerometerReadingEventArgs e ){
		MotionEvent( BBGameEvent.MotionAccel,0,(float)e.X,-(float)e.Y,(float)e.Z );
    }		
#endif
	
	//***** BBGame *****
	
	public override void SetUpdateRate( int hertz ){
		base.SetUpdateRate( hertz );
		ValidateUpdateTimer();
	}
	
	public override int SaveState( String state ){
#if WINDOWS
		IsolatedStorageFile file=IsolatedStorageFile.GetUserStoreForAssembly();
#else
		IsolatedStorageFile file=IsolatedStorageFile.GetUserStoreForApplication();
#endif
		if( file==null ) return -1;
		
		IsolatedStorageFileStream stream=file.OpenFile( ".monkeystate",FileMode.Create );
		if( stream==null ) return -1;

		StreamWriter writer=new StreamWriter( stream );
		writer.Write( state );
		writer.Close();
		
		return 0;
	}
	
	public override String LoadState(){
#if WINDOWS
		IsolatedStorageFile file=IsolatedStorageFile.GetUserStoreForAssembly();
#else
		IsolatedStorageFile file=IsolatedStorageFile.GetUserStoreForApplication();
#endif
		if( file==null ) return "";
		
		IsolatedStorageFileStream stream=file.OpenFile( ".monkeystate",FileMode.OpenOrCreate );
		if( stream==null ){
			return "";
		}

		StreamReader reader=new StreamReader( stream );
		String state=reader.ReadToEnd();
		reader.Close();
		
		return state;
	}
	
	public override bool PollJoystick( int port,float[] joyx,float[] joyy,float[] joyz,bool[] buttons ){
	
		if( port!=0 ) return false;
	
		GamePadState gs;

		if( _gamePadFound ){
			gs=GamePad.GetState( _gamePadIndex );
		}else{
			gs=GamePad.GetState( PlayerIndex.One );	//to shut up compiler.
			for( PlayerIndex i=PlayerIndex.One;i<=PlayerIndex.Four;++i ){
				gs=GamePad.GetState( i );
				if( !gs.IsConnected ) continue;
				if( gs.Buttons.A==ButtonState.Pressed || gs.Buttons.B==ButtonState.Pressed ||
				gs.Buttons.X==ButtonState.Pressed || gs.Buttons.Y==ButtonState.Pressed ||
				gs.Buttons.LeftShoulder==ButtonState.Pressed || gs.Buttons.RightShoulder==ButtonState.Pressed ||
				gs.Buttons.Back==ButtonState.Pressed || gs.Buttons.Start==ButtonState.Pressed ||
				gs.DPad.Left==ButtonState.Pressed || gs.DPad.Up==ButtonState.Pressed ||
				gs.DPad.Right==ButtonState.Pressed || gs.DPad.Down==ButtonState.Pressed ){
					_gamePadFound=true;
					_gamePadIndex=i;
					break;
				}
			}	
		}
		
		if( !_gamePadFound ) return false;
		
		joyx[0]=gs.ThumbSticks.Left.X;
		joyx[1]=gs.ThumbSticks.Right.X;
		joyy[0]=gs.ThumbSticks.Left.Y;
		joyy[1]=gs.ThumbSticks.Right.Y;
		joyz[0]=gs.Triggers.Left;
		joyz[1]=gs.Triggers.Right;
		
		buttons[0]=gs.Buttons.A==ButtonState.Pressed;
		buttons[1]=gs.Buttons.B==ButtonState.Pressed;
		buttons[2]=gs.Buttons.X==ButtonState.Pressed;
		buttons[3]=gs.Buttons.Y==ButtonState.Pressed;
		buttons[4]=gs.Buttons.LeftShoulder==ButtonState.Pressed;
		buttons[5]=gs.Buttons.RightShoulder==ButtonState.Pressed;
		buttons[6]=gs.Buttons.Back==ButtonState.Pressed;
		buttons[7]=gs.Buttons.Start==ButtonState.Pressed;
		buttons[8]=gs.DPad.Left==ButtonState.Pressed;
		buttons[9]=gs.DPad.Up==ButtonState.Pressed;
		buttons[10]=gs.DPad.Right==ButtonState.Pressed;
		buttons[11]=gs.DPad.Down==ButtonState.Pressed;
		
		return true;
	}
	
	public override void SetMouseVisible( bool visible ){
		_app.IsMouseVisible=visible;
	}
		
	public override FileStream OpenFile( String path,FileMode mode ){
	
		if( path.StartsWith( "monkey://internal/" ) ){
#if WINDOWS
			IsolatedStorageFile file=IsolatedStorageFile.GetUserStoreForAssembly();
#else
			IsolatedStorageFile file=IsolatedStorageFile.GetUserStoreForApplication();
#endif
			if( file==null ) return null;

			try{
				IsolatedStorageFileStream stream=file.OpenFile( path.Substring( 18 ),mode );
				return stream;
			}catch( Exception ){
			}
		}else{
			return base.OpenFile( path,mode );
		}
		return null;
	}
	
	public override Stream OpenInputStream( String path ){
	
		if( path.StartsWith( "monkey://data/" ) ){
			try{
				return TitleContainer.OpenStream( PathToContentPath( path ) );
			}catch( Exception ){
			}
		}else{
			return base.OpenInputStream( path );
		}
		return null;
	}
	
	public virtual Game GetXNAGame(){
		return _app;
	}
	
	public virtual String PathToContentPath( String path ){
		if( path.StartsWith("monkey://data/") ) return "Content/monkey/"+path.Substring( 14 );
		return "";
	}
	
	public virtual Texture2D LoadTexture2D( String path ){
		try{
			return _app.Content.Load<Texture2D>( PathToContentPath( path ) );
		}catch( Exception ){
		}
		return null;
	}

	public virtual SoundEffect LoadSoundEffect( String path ){
		try{
			return _app.Content.Load<SoundEffect>( PathToContentPath( path ) );
		}catch( Exception ){
		}
		return null;
	}
	
	public virtual Song LoadSong( String path ){
		try{
			return _app.Content.Load<Song>( PathToContentPath( path ) );
		}catch( Exception ){
		}
		return null;
	}
	
	
	//***** INTERNAL *****
	
	public override void Quit(){
		_exit=true;
		_app.Exit();
	}
	
	public override void SuspendGame(){
		base.SuspendGame();
		ValidateUpdateTimer();
	}
	
	public override void ResumeGame(){
		base.ResumeGame();
		ValidateUpdateTimer();
	}
	
	public override void UpdateGame(){
		PollKeyboard();
		PollMouse();
		PollTouch();
		base.UpdateGame();
	}
	
	public virtual void Run(){

		_app.IsMouseVisible=true;

		_autoSuspend=MonkeyConfig.MOJO_AUTO_SUSPEND_ENABLED=="1";
	
#if WINDOWS_PHONE
		if( MonkeyConfig.XNA_ACCELEROMETER_ENABLED=="1" ){
			_accelerometer=new Accelerometer();
			if( _accelerometer.State!=SensorState.NotSupported ){
				_accelerometer.ReadingChanged+=OnAccelerometerReadingChanged;
				_accelerometer.Start();
			}
        }
#endif

#if WINDOWS
		if( MonkeyConfig.XNA_WINDOW_FULLSCREEN!="1" ){
			_form=System.Windows.Forms.Form.FromHandle( _app.Window.Handle ) as System.Windows.Forms.Form;
			_form.FormClosing+=FormClosing;
		}
#endif		
		StartGame();
	}
	
	public virtual void Update( GameTime gameTime ){
		if( _exit ) return;
	
		if( PollSuspended() ) return;

		int updates;
		
		for( updates=0;updates<4;++updates ){
			_nextUpdate+=_updatePeriod;
			
			UpdateGame();
			if( !_app.IsFixedTimeStep || _updateRate==0 || _exit ) break;
			
			if( _nextUpdate-(double)Millisecs()>0 ) break;
		}
		
		if( updates==4 ) _nextUpdate=(double)Millisecs();
	}
	
	public virtual bool BeginDraw(){
		if( _exit ) return false;
		
		if( PollSuspended() && !_drawSuspended ) return false;

		_drawSuspended=false;
		
		return true;
	}

	public virtual void Draw( GameTime gameTime ){
		if( _exit ) return;
	
		RenderGame();
	}
	
#if WINDOWS	
	public virtual void FormClosing( object sender,System.Windows.Forms.FormClosingEventArgs e ){
		if( _exit ) return;
		
		KeyEvent( BBGameEvent.KeyDown,0x1b0 );
		KeyEvent( BBGameEvent.KeyUp,0x1b0 );
		
		e.Cancel=true;
	}
#endif
}
