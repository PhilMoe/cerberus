
public interface BBGameDelegate{

	void StartGame();
	void SuspendGame();
	void ResumeGame();
	void ResizeGame( int width,int height );
	void UpdateGame();
	void RenderGame();
	void UpdateGame();
	void RenderGame();
	void KeyEvent( int event,int data );
	void MouseEvent( int event,int data,float x,float y );
	void TouchEvent( int event,int data,float x,float y );
	void DiscardGraphics();
}

public class BBGame extends Activity implements GLSurfaceView.Renderer{

	static final int KEY_DOWN=1;
	static final int KEY_UP=2;
	static final int KEY_CHAR=3;
	static final int MOUSE_DOWN=4;
	static final int MOUSE_UP=5;
	static final int MOUSE_MOVE=6;
	static final int TOUCH_DOWN=7;
	static final int TOUCH_UP=8;
	static final int TOUCH_MOVE=9;

	static BBGame _game;
	
	GameView _view;	
	int _width;
	int _height;
	BBGameDelegate _delegate;
	int _updateRate;
	boolean _keyboardEnabled;

	//***** BBGame *****//	
	
	public void SetDelegate( BBGameDelegate delegate ){
		_delegate=delegate;
	}
	
	public void SetUpdateRate( int hertz ){
		_updateRate=hertz;
	}
	
	public void SetKeyboardEnabled( boolean enabled ){
		_keyboardEnabled=enabled;
	}
	
	public int Width(){
		return _width;
	}
	
	public int Height(){
		return _height;
	}
	
	public float AccelX(){
	}
	
	public float AccelY(){
	}
	
	public float AccelZ(){
	}
	
	public int Millisecs(){
		return (int)System.currentTimeMillis();
	}
	
	//***** INTERNAL *****
	synchronized public static BBGameDelegate Delegate(){
		return game._delegate;
	}
	
	//interface GLSurfaceView.Renderer
	synchronized public void onDrawFrame( GL10 gl ){
		Delegate().RenderGame();
	}
	
	//interface GLSurfaceView.Renderer
	synchronized public void onSurfaceChanged( GL10 gl,int width,int height ){
		_width=width;
		_height=height;
		Delegate().ResizeGame( width,height );
	}
	
	//interface GLSurfaceView.Renderer
	synchronized public void onSurfaceCreated( GL10 gl,EGLConfig config ){
		Delegate().DiscardGraphics();
	}
	
	public static class GameView extends GLSurfaceView implements GLSurfaceView.Renderer{

		public GameView( Context context ){
			super( context );
		}
		
		public GameView( Context context,AttributeSet attrs ){
			super( context,attrs );
		}
		
		public boolean dispatchKeyEventPreIme( KeyEvent event ){
			
			if( BBGame.game.keyboardEnabled ) {
				if( event.getKeyCode()==KeyEvent.KEYCODE_BACK ){
					if( event.getAction()==KeyEvent.ACTION_DOWN ){
						Delegate().KeyEvent( KEY_CHAR,27 );
					}
					return true;
				}
			}else{
				if( event.getKeyCode()==KeyEvent.KEYCODE_BACK ){
					if( event.getAction()==KeyEvent.ACTION_DOWN ){
						Delegate().KeyEvent( KEY_DOWN,27 );
					}else if( event.getAction()==KeyEvent.ACTION_UP ){
						Delegate().KeyEvent( KEY_UP,27 );
					}
					return true;
				}
			}
			return false;
		}
		
		public boolean onKeyDown( int key,KeyEvent event ){
			if( BBGame.game._keyboardEnabled ) return false;
			
			if( event.getKeyCode()==KeyEvent.KEYCODE_DEL ){
				Delegate().KeyEvent( KEY_CHAR,8 );
			}else{
				int chr=event.getUnicodeChar();
				if( chr!=0 ){
					if( chr==10 ) chr=13;
					Delegate().KeyEvent( KEY_CHAR,chr );
				}
			}
			return true;
		}
		
		public boolean onKeyMultiple( int keyCode,int repeatCount,KeyEvent event ){
			if( !BBGame.game._keyboardEnabled ) return false;
		
			String str=event.getCharacters();
			for( int i=0;i<str.length();++i ){
				int chr=str.charAt( i );
				if( chr!=0 ){
					if( chr==10 ) chr=13;
					Delegate().KeyEvent( KEY_CHAR,chr );
				}
			}
			return true;
		}
		
		//fields for touch event handling
		boolean useMulti,checkedMulti;
		Method getPointerCount,getPointerId,getX,getY;
		Object args1[]=new Object[1];
		
		public boolean onTouchEvent( MotionEvent event ){
			if( app==null ) return false;
		
			if( !checkedMulti ){
				//Check for multi-touch support
				//
				try{
					Class cls=event.getClass();
					Class intClass[]=new Class[]{ Integer.TYPE };
					getPointerCount=cls.getMethod( "getPointerCount" );
					getPointerId=cls.getMethod( "getPointerId",intClass );
					getX=cls.getMethod( "getX",intClass );
					getY=cls.getMethod( "getY",intClass );
					useMulti=true;
				}catch( NoSuchMethodException ex ){
					useMulti=false;
				}
				checkedMulti=true;
			}
			
			if( !useMulti ){
				//mono-touch version...
				//
				int action=event.getAction();
				float x=event.getX(),y=event.getY();
				
				switch( action ){
				case MotionEvent.ACTION_DOWN:
					Delegate().TouchEvent( TOUCH_DOWN,0,x,y );
					break;
				case MotionEvent.ACTION_UP:
					Delegate().TouchEvent( TOUCH_UP,0,x,y );
				case MotionEvent.ACTION_MOVE:
					Delegate().TouchEvent( TOUCH_MOVE,0,x,y );
					break;
				}
				return true;
			}

			try{

				//multi-touch version...
				//
				final int ACTION_DOWN=0;
				final int ACTION_UP=1;
				final int ACTION_MOVE=2;
				final int ACTION_POINTER_DOWN=5;
				final int ACTION_POINTER_UP=6;
				final int ACTION_POINTER_ID_SHIFT=8;
				final int ACTION_MASK=255;
				
				gxtkInput input=app.input;
				
				int action=event.getAction();
				float x=event.getX(),y=event.getY();
				
				int maskedAction=action & ACTION_MASK;
				int pid=0;
				
				if( maskedAction==ACTION_POINTER_DOWN || maskedAction==ACTION_POINTER_UP ){
					args1[0]=Integer.valueOf( action>>ACTION_POINTER_ID_SHIFT );
					pid=((Integer)getPointerId.invoke( event,args1 )).intValue();
				}else{
					args1[0]=Integer.valueOf( 0 );
					pid=((Integer)getPointerId.invoke( event,args1 )).intValue();
				}
				
				switch( maskedAction ){
				case ACTION_DOWN:
				case ACTION_POINTER_DOWN:
					Delegate().TouchEvent( TOUCH_DOWN,pid,x,y );
					break;
				case ACTION_UP:
				case ACTION_POINTER_UP:
					Delegate().TouchEvent( TOUCH_UP,pid,x,y );
					break;
				case ACTION_MOVE:
					Delegate().TouchEvent( TOUCH_MOVE,pid,x,y );
					break;
				}

			}catch( Exception e ){
			}
	
			return true;
		}
		
		public void onDrawFrame( GL10 gl ){
		}
	
		public void onSurfaceChanged( GL10 gl,int width,int height ){
		}
	
		public void onSurfaceCreated( GL10 gl,EGLConfig config ){
		}
	}
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate( Bundle savedInstanceState ){	//onStart
		super.onCreate( savedInstanceState );
		
		System.setOut( new PrintStream( new LogTool() ) );
		
		setContentView( R.layout.main );

		_view=(GameView)findViewById( R.id.GameView );
		_view.setFocusableInTouchMode( true );
		_view.requestFocus();
		
		setVolumeControlStream( AudioManager.STREAM_MUSIC );
			
		try{
		
			bb_.bbInit();
			
			bb_.bbMain();
			
			if( _delegate==null ) System.exit( 0 );
			
			if( MonkeyConfig.OPENGL_GLES20_ENABLED.equals( "1" ) ){
				
				//view.setEGLContextClientVersion( 2 );	//API 8 only!
				//
				try{
					Class clas=view.getClass();
					Class parms[]=new Class[]{ Integer.TYPE };
					Method setVersion=clas.getMethod( "setEGLContextClientVersion",parms );
					Object args[]=new Object[1];
					args[0]=Integer.valueOf( 2 );
					setVersion.invoke( view,args );
				}catch( NoSuchMethodException ex ){
				}
			}

			_view.setRenderer( app );
			_view.setRenderMode( GLSurfaceView.RENDERMODE_WHEN_DIRTY );
			_view.requestRender();

		}catch( Throwable t ){
		
			new gxtkAlert( t );
		}
	}
	
	@Override
	public void onRestart(){
		super.onRestart();
	}
	
	@Override
	public void onStart(){
		super.onStart();
		_delegate.StartGame();
	}
	
	@Override
	public void onResume(){
		super.onResume();
		view.onResume();
		_delegate.ResumeGame();
	}
	
	@Override 
	public void onPause(){
		super.onPause();
		_delegate.SuspendGame();
		view.onPause();
	}

	@Override
	public void onStop(){
		super.onStop();
	}
	
	@Override
	public void onDestroy(){
		super.onDestroy();
	}

	static class LogTool extends OutputStream{

		private ByteArrayOutputStream out=new ByteArrayOutputStream();
	  
		@Override
		public void write( int b ) throws IOException{
			if( b==(int)'\n' ){
				Log.i( "[Monkey]",new String( out.toByteArray() ) );
				out=new ByteArrayOutputStream();
			}else{
				out.write( b );
			}
		}
	}
}
