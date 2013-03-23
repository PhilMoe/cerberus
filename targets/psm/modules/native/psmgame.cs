
public class BBPsmGame : BBGame{

	static BBPsmGame _psmGame;

	protected GraphicsContext _gc;
	
	int[] _touchId=new int[32];
	float[] _touchX=new float[32];
	float[] _touchY=new float[32];
	
	public BBPsmGame(){
		_psmGame=this;

		_gc=new GraphicsContext();
		
		for( int i=0;i<32;++i ) _touchId[i]=-1;
	}
	
	public static BBPsmGame PsmGame(){
		return _psmGame;
	}
	
	void PollTouch(){
	
		float gw=_gc.GetViewport().Width;
		float gh=_gc.GetViewport().Height;
	
		List<TouchData> touchData=Touch.GetData( 0 );
		
		foreach( TouchData td in touchData ){
				
			if( td.Status==TouchStatus.None ) continue;
		
			float x=(td.X+0.5f)*gw;
			float y=(td.Y+0.5f)*gh;
		
			int pid;
			for( pid=0;pid<32 && _touchId[pid]!=td.ID;++pid ){}
			
			int ev=BBGameEvent.None;
			
			switch( td.Status ){
			case TouchStatus.Down:
				if( pid!=32 ) break;
				for( pid=0;pid<32 && _touchId[pid]!=-1;++pid ){}
				if( pid==32 ) break;
				_touchId[pid]=td.ID;
				ev=BBGameEvent.TouchDown;
				break;
			case TouchStatus.Up:case TouchStatus.Canceled:
				if( pid==32 ) break;
				_touchId[pid]=-1;
				ev=BBGameEvent.TouchUp;
				break;
			case TouchStatus.Move:
				if( pid!=32 && (x!=_touchX[pid] || y!=_touchY[pid]) ) ev=BBGameEvent.TouchMove;
				break;
			}
			if( ev!=BBGameEvent.None ){
				_touchX[pid]=x;
				_touchY[pid]=y;
				TouchEvent( ev,pid,x,y );
			}
		}
	}
	
	void PollMotion(){
		MotionData md=Motion.GetData( 0 );
		MotionEvent( BBGameEvent.MotionAccel,0,md.Acceleration.X,-md.Acceleration.Y,md.Acceleration.Z );
	}	
	
	//***** BBGame *****
	
	public override int SaveState( String state ){
		try{
			File.WriteAllText( "/Documents/.monkeystate",state );
			return 0;
		}catch( IOException ex ){
		}
		return -1;
	}
	
	public override String LoadState(){
		try{
			return File.ReadAllText( "/Documents/.monkeystate" );
		}catch( IOException ex ){
		}
		return "";
	}

	public override bool PollJoystick( int port,float[] joyx,float[] joyy,float[] joyz,bool[] buttons ){
	
		if( port!=0 ) return false;
		
		GamePadData gd=GamePad.GetData( port );
		
		joyx[0]=gd.AnalogLeftX;
		joyy[0]=-gd.AnalogLeftY;

		joyx[1]=gd.AnalogRightX;
		joyy[1]=-gd.AnalogRightY;
		
		GamePadButtons down=gd.ButtonsDown;
		
		buttons[0]=(down & GamePadButtons.Cross)!=0;
		buttons[1]=(down & GamePadButtons.Circle)!=0;
		buttons[2]=(down & GamePadButtons.Square)!=0;
		buttons[3]=(down & GamePadButtons.Triangle)!=0;
		buttons[4]=(down & GamePadButtons.L)!=0;
		buttons[5]=(down & GamePadButtons.R)!=0;
		buttons[6]=(down & GamePadButtons.Back)!=0;
		buttons[7]=(down & GamePadButtons.Start)!=0;
		buttons[8]=(down & GamePadButtons.Left)!=0;
		buttons[9]=(down & GamePadButtons.Up)!=0;
		buttons[10]=(down & GamePadButtons.Right)!=0;
		buttons[11]=(down & GamePadButtons.Down)!=0;
		
		return true;
	}
	
	public override String PathToFilePath( String path ){
		if( path.StartsWith( "monkey://data/" ) ){
			return "/Application/data/"+path.Substring( 14 );
		}else if( path.StartsWith( "monkey://internal/" ) ){
			return "/Documents/"+path.Substring( 18 );
		}
		return "";
	}

	//Hacklet for PSM - doesn't let you use File.Open for files in data/
	public override FileStream OpenFile( String path,FileMode mode ){
	
		if( mode!=FileMode.Open ) return base.OpenFile( path,mode );
		
		try{
			return File.OpenRead( PathToFilePath( path ) );
		}catch( Exception ex ){
		}
		return null;
	}

	public virtual GraphicsContext GetGraphicsContext(){
		return _gc;
	}
	
	public virtual Texture2D LoadTexture2D( String path ){
		try{
			return new Texture2D( PathToFilePath( path ),false );
		}catch( Exception ex ){
		}
		return null;
	}

	public virtual Sound LoadSound( String path ){
		try{
			return new Sound( PathToFilePath( path ) );
		}catch( Exception ex ){
		}
		return null;
	}
	
	public virtual Bgm LoadBgm( String path ){
		try{
			return new Bgm( PathToFilePath( path ) );
		}catch( Exception ex ){
		}
		return null;
	}
	
	//***** INTERNAL *****
		
	public override void Quit(){
		System.Environment.Exit( 0 );
	}
	
	public override void UpdateGame(){
		PollTouch();
		PollMotion();
		base.UpdateGame();
	}
	
	public virtual void Run(){
		
		StartGame();
		RenderGame();
		
		long time=_stopwatch.ElapsedMilliseconds;
		
		for(;;){
		
			SystemEvents.CheckEvents();
			
			if( _updateRate==0 ) continue;
			
			UpdateGame();
			RenderGame();
			
			if( _updateRate==0 || _updateRate>=60 ) continue;
			
			long period=1000/_updateRate;
			
			while( _stopwatch.ElapsedMilliseconds-time<period ){
				SystemEvents.CheckEvents();
			}
			
			time+=period;
		}
	}
}
