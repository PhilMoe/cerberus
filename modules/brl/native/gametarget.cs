
public class BBGameEvent{
	public const int None=0;
	public const int KeyDown=1;
	public const int KeyUp=2;
	public const int KeyChar=3;
	public const int MouseDown=4;
	public const int MouseUp=5;
	public const int MouseMove=6;
	public const int TouchDown=7;
	public const int TouchUp=8;
	public const int TouchMove=9;
	public const int MotionAccel=10;
}

public class BBGameDelegate{
	public virtual void StartGame(){}
	public virtual void SuspendGame(){}
	public virtual void ResumeGame(){}
	public virtual void UpdateGame(){}
	public virtual void RenderGame(){}
	public virtual void KeyEvent( int ev,int data ){}
	public virtual void MouseEvent( int ev,int data,float x,float y,float z ){}
	public virtual void TouchEvent( int ev,int data,float x,float y ){}
	public virtual void MotionEvent( int ev,int data,float x,float y,float z ){}
	public virtual void DiscardGraphics(){}
}

public class BBDisplayMode{
	public int width;
	public int height;
	public int format;
	public int hertz;
	public int flags;
	
	public BBDisplayMode( int width=0,int height=0,int format=0,int hertz=0,int flags=0 ){
		this.width=width;
		this.height=height;
		this.format=format;
		this.hertz=hertz;
		this.flags=flags;
	}
}

public class BBGame{

	protected static BBGame _game;
	
	protected BBGameDelegate _delegate;
	protected bool _keyboardEnabled;
	protected int _updateRate;
	protected bool _debugExs;
	protected bool _started;
	protected bool _suspended;
	protected Stopwatch _stopwatch;
	
	public BBGame(){
		_game=this;
		_debugExs=(CerberusConfig.CONFIG=="debug");
		_stopwatch=Stopwatch.StartNew();
		
	}
	
	public static BBGame Game(){
		return _game;
	}
	
	public virtual void SetDelegate( BBGameDelegate delegate_ ){
		_delegate=delegate_;
	}
	
	public BBGameDelegate Delegate(){
		return _delegate;
	}
	
	public virtual void SetKeyboardEnabled( bool enabled ){
		_keyboardEnabled=enabled;
	}
	
	public virtual void SetUpdateRate( int hertz ){
		_updateRate=hertz;
	}
	
	public virtual int Millisecs(){
		return (int)_stopwatch.ElapsedMilliseconds;
	}
	
	public virtual void GetDate( int[] date ){
		int n=date.Length;
		if( n>0 ){
			DateTime t=DateTime.Now;
			date[0]=t.Year;
			if( n>1 ){
				date[1]=t.Month;
				if( n>2 ){
					date[2]=t.Day;
					if( n>3 ){
						date[3]=t.Hour;
						if( n>4 ){
							date[4]=t.Minute;
							if( n>5 ){
								date[5]=t.Second;
								if( n>6 ){
									date[6]=t.Millisecond;
								}
							}
						}
					}
				}
			}
		}
	}
	
	public virtual String CurrentDate(){
		return DateTime.Now.ToString( "dd MMM yyyy" );
	}
	
	public virtual String CurrentTime(){
		return DateTime.Now.ToString( "HH\\:mm\\:ss" );
	}
	
	public virtual int SaveState( String state ){
		return -1;
	}
	
	public virtual String LoadState(){
		return "";
	}
	
	public virtual String LoadString( String path ){

		Stream stream=OpenInputStream( path );
		if( stream==null ) return "";
		
		StreamReader reader=new StreamReader( stream );
		String text=reader.ReadToEnd();
		reader.Close();

		return text;
	}
	
	public virtual int CountJoysticks( bool update ){
		return 0;
	}
	
	public virtual bool PollJoystick( int port,float[] joyx,float[] joyy,float[] joyz,bool[] buttons ){
		return false;
	}
	
	public virtual void OpenUrl( String url ){
	}
	
	public virtual void SetMouseVisible( bool visible ){
	}
	
	public virtual int GetDeviceWidth(){
		return 0;
	}
	
	public virtual int GetDeviceHeight(){
		return 0;
	}
	
	public virtual void SetDeviceWindow( int width,int height,int flags ){
	}
	
	public virtual BBDisplayMode[] GetDisplayModes(){
		return new BBDisplayMode[0];
	}
	
	public virtual BBDisplayMode GetDesktopMode(){
		return null;
	}
	
	public virtual void SetSwapInterval( int interval ){
	}
	
	public virtual String PathToFilePath( String path ){
		return "";
	}
	
	//***** C# extensions *****
	
	public virtual FileStream OpenFile( String path,FileMode mode ){
		try{
			return new FileStream( PathToFilePath( path ),mode );
		}catch( Exception ex ){
		}
		return null;
	}
	
	public virtual Stream OpenInputStream( String path ){
		return OpenFile( path,FileMode.Open );
	}
	
	public virtual byte[] LoadData( String path ){
	
		Stream stream=OpenInputStream( path );
		if( stream==null ) return null;
		
		//fixme: stream may not have a Length (will always for now though).
		//
		int len=(int)stream.Length;
		byte[] buf=new byte[len];
		int n=stream.Read( buf,0,len );
		stream.Close();
		if( n==len ) return buf;
	
		return null;
	}

	//***** INTERNAL *****
	
	public virtual void Quit(){
		_delegate=new BBGameDelegate();
	}
	
	public virtual bool Die( Exception ex ){
	
		if( ex.Message=="" ){
			Quit();
			return false;
		}
		if( _debugExs ){
			bb_std_lang.Print( "Cerberus Runtime Error : "+ex.Message );
			bb_std_lang.Print( bb_std_lang.StackTrace() );
		}
		return true;
	}
	
	public virtual void StartGame(){
	
		if( _started ) return;
		_started=true;
		
		try{
			_delegate.StartGame();
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void SuspendGame(){
	
		if( !_started || _suspended ) return;
		_suspended=true;
		
		try{
			_delegate.SuspendGame();
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void ResumeGame(){
	
		if( !_started || !_suspended ) return;
		_suspended=false;
		
		try{
			_delegate.ResumeGame();
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void UpdateGame(){
	
		if( !_started || _suspended ) return;
		
		try{
			_delegate.UpdateGame();
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void RenderGame(){
	
		if( !_started ) return;
		
		try{
			_delegate.RenderGame();
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void KeyEvent( int ev,int data ){

		if( !_started ) return;
		
		try{
			_delegate.KeyEvent( ev,data );
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void MouseEvent( int ev,int data,float x,float y,float z ){

		if( !_started ) return;
		
		try{
			_delegate.MouseEvent( ev,data,x,y,z );
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void TouchEvent( int ev,int data,float x,float y ){

		if( !_started ) return;
		
		try{
			_delegate.TouchEvent( ev,data,x,y );
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
	
	public virtual void MotionEvent( int ev,int data,float x,float y,float z ){

		if( !_started ) return;
		
		try{
			_delegate.MotionEvent( ev,data,x,y,z );
		}catch( Exception ex ){
			if( Die( ex ) ) throw;
		}
	}
}
