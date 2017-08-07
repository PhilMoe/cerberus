
import android.os.*;
import android.app.*;
import android.media.*;
import android.view.*;
import android.graphics.*;
import android.content.*;
import android.util.*;
import android.hardware.*;
import android.net.*;
import android.widget.*;
import android.view.inputmethod.*;
import android.content.res.*;
import android.opengl.*;
import android.text.*;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

import com.cerberus.LangUtil;

class ActivityDelegate{

	public void onStart(){
	}
	public void onRestart(){
	}
	public void onResume(){
	}
	public void onPause(){
	}
	public void onStop(){
	}
	public void onDestroy(){
	}
	public void onActivityResult( int requestCode,int resultCode,Intent data ){
	}
	public void onNewIntent( Intent intent ){
	}
}

class BBAndroidGame extends BBGame implements GLSurfaceView.Renderer,SensorEventListener{

	static BBAndroidGame _androidGame;
	
	Activity _activity;
	GameView _view;
	
	List<ActivityDelegate> _activityDelegates=new LinkedList<ActivityDelegate>();
	
	int _reqCode;
	
	Display _display;
	
	long _nextUpdate;
	
	long _updatePeriod;
	
	boolean _canRender;
	
	float[] _joyx=new float[2];
	float[] _joyy=new float[2];
	float[] _joyz=new float[2];
	boolean[] _buttons=new boolean[32];
	
	public BBAndroidGame( Activity activity,GameView view ){
		_androidGame=this;

		_activity=activity;
		_view=view;
		
		_display=_activity.getWindowManager().getDefaultDisplay();
		
		System.setOut( new PrintStream( new LogTool() ) );
	}
	
	public static BBAndroidGame AndroidGame(){

		return _androidGame;
	}
	
	//***** LogTool ******	

	static class LogTool extends OutputStream{
	
		ByteArrayOutputStream out=new ByteArrayOutputStream();
		
		@Override
		public void write( int b ) throws IOException{
			if( b==(int)'\n' ){
				Log.i( "[Cerberus]",new String( this.out.toByteArray() ) );
				this.out=new ByteArrayOutputStream();
			}else{
				this.out.write(b);
			}
		}
	}
	
	void ValidateUpdateTimer(){
		_nextUpdate=0;
		_updatePeriod=0;
		if( _updateRate!=0 ) _updatePeriod=1000000000/_updateRate;
		
	}
	
	//***** GameView *****
	
	public static class GameView extends GLSurfaceView{
	
		Object args1[]=new Object[1];
		float[] _touchX=new float[32];
		float[] _touchY=new float[32];

		boolean _useMulti;
		Method _getPointerCount,_getPointerId,_getX,_getY;
		
		boolean _useGamepad;
		Method _getSource,_getAxisValue;

		void init(){
		
			//get multi-touch methods
			try{
				Class cls=Class.forName( "android.view.MotionEvent" );
				Class intClass[]=new Class[]{ Integer.TYPE };
				_getPointerCount=cls.getMethod( "getPointerCount" );
				_getPointerId=cls.getMethod( "getPointerId",intClass );
				_getX=cls.getMethod( "getX",intClass );
				_getY=cls.getMethod( "getY",intClass );
				_useMulti=true;
			}catch( Exception ex ){
			}
			
			if( CerberusConfig.ANDROID_GAMEPAD_ENABLED.equals( "1" ) ){
				try{
					//get gamepad methods
					Class cls=Class.forName( "android.view.MotionEvent" );
					Class intClass[]=new Class[]{ Integer.TYPE };
					_getSource=cls.getMethod( "getSource" );
					_getAxisValue=cls.getMethod( "getAxisValue",intClass );
					_useGamepad=true;
				}catch( Exception ex ){
				}
			}
		}

		public GameView( Context context ){
			super( context );
			init();
		}
		
		public GameView( Context context,AttributeSet attrs ){
			super( context,attrs );
			init();
		}
		
		/*
		public InputConnection onCreateInputConnection( EditorInfo outAttrs ){
			//voodoo to disable various undesirable soft keyboard features such as predictive text and fullscreen mode.
			outAttrs.inputType=InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
			outAttrs.imeOptions=EditorInfo.IME_FLAG_NO_FULLSCREEN|EditorInfo.IME_FLAG_NO_EXTRACT_UI;			
			return null;
		}
		*/
		
		public InputConnection onCreateInputConnection( EditorInfo outAttrs ){
			//voodoo to disable various undesirable soft keyboard features such as predictive text and fullscreen mode.
			outAttrs.inputType=InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
			outAttrs.imeOptions=EditorInfo.IME_FLAG_NO_FULLSCREEN|EditorInfo.IME_FLAG_NO_EXTRACT_UI;
			outAttrs.initialSelStart=-1;
			outAttrs.initialSelEnd=-1;
			return new BackspaceInputConnection( this,false );
		}
				
		//Yet more voodoo courtesy of secondgear
		private class BackspaceInputConnection extends BaseInputConnection {
					
			public BackspaceInputConnection( View targetView,boolean fullEditor ){
				super( targetView,fullEditor );
			}
		
			public boolean deleteSurroundingText( int beforeLength,int afterLength ){       
				if( beforeLength==1 && afterLength==0 ){
					return	super.sendKeyEvent( new KeyEvent( KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DEL ) ) &&
							super.sendKeyEvent( new KeyEvent( KeyEvent.ACTION_UP,KeyEvent.KEYCODE_DEL ) );
				}
				return super.deleteSurroundingText( beforeLength,afterLength );
			}			
		}		
		
		//View event handling
		public boolean dispatchKeyEventPreIme( KeyEvent event ){

			if( _useGamepad ){
				int button=-1;
				switch( event.getKeyCode() ){
				case 96: button=0;break;	//A
				case 97: button=1;break;	//B
				case 99: button=2;break;	//X
				case 100:button=3;break;	//Y
				case 102:button=4;break;	//LB
				case 103:button=5;break;	//RB
				case 108:button=7;break;	//START
				case 21: button=8;break;	//LEFT
				case 19: button=9;break;	//UP
				case 22: button=10;break;	//RIGHT
				case 20: button=11;break;	//DOWN
				}
				if( button!=-1 ){
					_androidGame._buttons[button]=(event.getAction()==KeyEvent.ACTION_DOWN);
					return true;
				}
			}
			
			//Convert back button to ESC in soft keyboard mode...
			if( _androidGame._keyboardEnabled ){
				if( event.getKeyCode()==KeyEvent.KEYCODE_BACK ){
					if( event.getAction()==KeyEvent.ACTION_DOWN ){
						_androidGame.KeyEvent( BBGameEvent.KeyChar,27 );
					}
					return true;
				}
			}
			return false;
		}
		
		public boolean onKeyDown( int key,KeyEvent event ){
		
			int vkey=-1;
			switch( event.getKeyCode() ){
			case KeyEvent.KEYCODE_MENU:vkey=0x1a1;break;
			case KeyEvent.KEYCODE_SEARCH:vkey=0x1a3;break;
			}
			if( vkey!=-1 ){
				_androidGame.KeyEvent( BBGameEvent.KeyDown,vkey );
				_androidGame.KeyEvent( BBGameEvent.KeyUp,vkey );
				return true;
			}
			
			if( !_androidGame._keyboardEnabled ) return false;
			
			if( event.getKeyCode()==KeyEvent.KEYCODE_DEL ){
				_androidGame.KeyEvent( BBGameEvent.KeyChar,8 );
			}else{
				int chr=event.getUnicodeChar();
				if( chr!=0 ){
					_androidGame.KeyEvent( BBGameEvent.KeyChar,chr==10 ? 13 : chr );
				}
			}
			return true;
		}
		
		public boolean onKeyMultiple( int keyCode,int repeatCount,KeyEvent event ){
			if( !_androidGame._keyboardEnabled ) return false;
		
			String str=event.getCharacters();
			for( int i=0;i<str.length();++i ){
				int chr=str.charAt( i );
				if( chr!=0 ){
					_androidGame.KeyEvent( BBGameEvent.KeyChar,chr==10 ? 13 : chr );
				}
			}
			return true;
		}
		
		public boolean onTouchEvent( MotionEvent event ){
		
			if( !_useMulti ){
				//mono-touch version...
				//
				switch( event.getAction() ){
				case MotionEvent.ACTION_DOWN:
					_androidGame.TouchEvent( BBGameEvent.TouchDown,0,event.getX(),event.getY() );
					break;
				case MotionEvent.ACTION_UP:
					_androidGame.TouchEvent( BBGameEvent.TouchUp,0,event.getX(),event.getY() );
					break;
				case MotionEvent.ACTION_MOVE:
					_androidGame.TouchEvent( BBGameEvent.TouchMove,0,event.getX(),event.getY() );
					break;
				}
				return true;
			}
	
			try{
	
				//multi-touch version...
				//
				final int ACTION_DOWN=0;
				final int ACTION_UP=1;
				final int ACTION_POINTER_DOWN=5;
				final int ACTION_POINTER_UP=6;
				final int ACTION_POINTER_INDEX_SHIFT=8;
				final int ACTION_MASK=255;
				
				int index=-1;
				int action=event.getAction();
				int masked=action & ACTION_MASK;
				
				if( masked==ACTION_DOWN || masked==ACTION_POINTER_DOWN || masked==ACTION_UP || masked==ACTION_POINTER_UP ){
	
					index=action>>ACTION_POINTER_INDEX_SHIFT;
					
					args1[0]=Integer.valueOf( index );
					int pid=((Integer)_getPointerId.invoke( event,args1 )).intValue();
	
					float x=_touchX[pid]=((Float)_getX.invoke( event,args1 )).floatValue();
					float y=_touchY[pid]=((Float)_getY.invoke( event,args1 )).floatValue();
					
					if( masked==ACTION_DOWN || masked==ACTION_POINTER_DOWN ){
						_androidGame.TouchEvent( BBGameEvent.TouchDown,pid,x,y );
					}else{
						_androidGame.TouchEvent( BBGameEvent.TouchUp,pid,x,y );
					}
				}
	
				int pointerCount=((Integer)_getPointerCount.invoke( event )).intValue();
			
				for( int i=0;i<pointerCount;++i ){
					if( i==index ) continue;
	
					args1[0]=Integer.valueOf( i );
					int pid=((Integer)_getPointerId.invoke( event,args1 )).intValue();
	
					float x=((Float)_getX.invoke( event,args1 )).floatValue();
					float y=((Float)_getY.invoke( event,args1 )).floatValue();
	
					if( x!=_touchX[pid] || y!=_touchY[pid] ){
						_touchX[pid]=x;
						_touchY[pid]=y;
						_androidGame.TouchEvent( BBGameEvent.TouchMove,pid,x,y );
					}
				}
			}catch( Exception ex ){
			}
	
			return true;
		}
		
		//New! Dodgy gamepad support...
		public boolean onGenericMotionEvent( MotionEvent event ){
		
			if( !_useGamepad ) return false;
			
			try{
				int source=((Integer)_getSource.invoke( event )).intValue();

				if( (source&16)==0 ) return false;
			
				BBAndroidGame g=_androidGame;
			
				args1[0]=Integer.valueOf( 0  );g._joyx[0]=((Float)_getAxisValue.invoke( event,args1 )).floatValue();
				args1[0]=Integer.valueOf( 1  );g._joyy[0]=((Float)_getAxisValue.invoke( event,args1 )).floatValue();
				args1[0]=Integer.valueOf( 17 );g._joyz[0]=((Float)_getAxisValue.invoke( event,args1 )).floatValue();
				
				args1[0]=Integer.valueOf( 11 );g._joyx[1]=((Float)_getAxisValue.invoke( event,args1 )).floatValue();
				args1[0]=Integer.valueOf( 14 );g._joyy[1]=((Float)_getAxisValue.invoke( event,args1 )).floatValue();
				args1[0]=Integer.valueOf( 18 );g._joyz[1]=((Float)_getAxisValue.invoke( event,args1 )).floatValue();
				
				return true;
				
			}catch( Exception ex ){
			}

			return false;
		}
	}
	
	//***** BBGame ******
	
	public int GetDeviceWidth(){
		return _view.getWidth();
	}
	
	public int GetDeviceHeight(){
		return _view.getHeight();
	}
	
	public void SetKeyboardEnabled( boolean enabled ){
		super.SetKeyboardEnabled( enabled );

		InputMethodManager mgr=(InputMethodManager)_activity.getSystemService( Context.INPUT_METHOD_SERVICE );
		
		if( _keyboardEnabled ){
			// Hack for someone's phone...My LG or Samsung don't need it...
			mgr.hideSoftInputFromWindow( _view.getWindowToken(),0 );
			mgr.showSoftInput( _view,0 );		//0 is 'magic'! InputMethodManager.SHOW_IMPLICIT does weird things...
		}else{
			mgr.hideSoftInputFromWindow( _view.getWindowToken(),0 );
		}
	}
	
	public void SetUpdateRate( int hertz ){
		super.SetUpdateRate( hertz );
		ValidateUpdateTimer();
	}	

	public int SaveState( String state ){
		SharedPreferences prefs=_activity.getPreferences( 0 );
		SharedPreferences.Editor editor=prefs.edit();
		editor.putString( ".cerberusstate",state );
		editor.commit();
		return 1;
	}
	
	public String LoadState(){
		SharedPreferences prefs=_activity.getPreferences( 0 );
		String state=prefs.getString( ".cerberusstate","" );
		if( state.equals( "" ) ) state=prefs.getString( "gxtkAppState","" );
		return state;
	}
	
	static public String LoadState_V66b(){
		SharedPreferences prefs=_androidGame._activity.getPreferences( 0 );
		return prefs.getString( "gxtkAppState","" );
	}
	
	static public void SaveState_V66b( String state ){
		SharedPreferences prefs=_androidGame._activity.getPreferences( 0 );
		SharedPreferences.Editor editor=prefs.edit();
		editor.putString( "gxtkAppState",state );
		editor.commit();
	}
	
	public boolean PollJoystick( int port,float[] joyx,float[] joyy,float[] joyz,boolean[] buttons ){
		if( port!=0 ) return false;
		joyx[0]=_joyx[0];joyy[0]=_joyy[0];joyz[0]=_joyz[0];
		joyx[1]=_joyx[1];joyy[1]=_joyy[1];joyz[1]=_joyz[1];
		for( int i=0;i<32;++i ) buttons[i]=_buttons[i];
		return true;
	}
	
	public void OpenUrl( String url ){
		Intent browserIntent=new Intent( Intent.ACTION_VIEW,android.net.Uri.parse( url ) );
		_activity.startActivity( browserIntent );
	}
	
	String PathToFilePath( String path ){
		if( !path.startsWith( "cerberus://" ) ){
			return path;
		}else if( path.startsWith( "cerberus://internal/" ) ){
			File f=_activity.getFilesDir();
			if( f!=null ) return f+"/"+path.substring(20);
		}else if( path.startsWith( "cerberus://external/" ) ){
			File f=Environment.getExternalStorageDirectory();
			if( f!=null ) return f+"/"+path.substring(20);
		}
		return "";
	}

	String PathToAssetPath( String path ){
		if( path.startsWith( "cerberus://data/" ) ) return "cerberus/"+path.substring(16);
		return "";
	}

	public InputStream OpenInputStream( String path ){
		if( !path.startsWith( "cerberus://data/" ) ) return super.OpenInputStream( path );
		try{
			return _activity.getAssets().open( PathToAssetPath( path ) );
		}catch( IOException ex ){
		}
		return null;
	}

	public Activity GetActivity(){
		return _activity;
	}

	public GameView GetGameView(){
		return _view;
	}
	
	public void AddActivityDelegate( ActivityDelegate delegate ){
		if( _activityDelegates.contains( delegate ) ) return;
		_activityDelegates.add( delegate );
	}
	
	public int AllocateActivityResultRequestCode(){
		return ++_reqCode;
	}
	
	public void RemoveActivityDelegate( ActivityDelegate delegate ){
		_activityDelegates.remove( delegate );
	}

	public Bitmap LoadBitmap( String path ){
		try{
			InputStream in=OpenInputStream( path );
			if( in==null ) return null;

			BitmapFactory.Options opts=new BitmapFactory.Options();
			opts.inPreferredConfig=Bitmap.Config.ARGB_8888;

			Bitmap bitmap=BitmapFactory.decodeStream( in,null,opts );
			in.close();
			
			return bitmap;
		}catch( IOException e ){
		}
		return null;
	}

	public int LoadSound( String path,SoundPool pool ){
		try{
			if( path.startsWith( "cerberus://data/" ) ){
				return pool.load( _activity.getAssets().openFd( PathToAssetPath( path ) ),1 );
			}else{
				return pool.load( PathToFilePath( path ),1 );
			}
		}catch( IOException ex ){
		}
		return 0;
	}
	
	public MediaPlayer OpenMedia( String path ){
		try{
			MediaPlayer mp;
			
			if( path.startsWith( "cerberus://data/" ) ){
				AssetFileDescriptor fd=_activity.getAssets().openFd( PathToAssetPath( path ) );
				mp=new MediaPlayer();
				mp.setDataSource( fd.getFileDescriptor(),fd.getStartOffset(),fd.getLength() );
				mp.prepare();
				fd.close();
			}else{
				mp=new MediaPlayer();
				mp.setDataSource( PathToFilePath( path ) );
				mp.prepare();
			}
			return mp;
			
		}catch( IOException ex ){
		}
		return null;
	}
	
	//***** INTERNAL *****
	
	public void SuspendGame(){
		super.SuspendGame();
		ValidateUpdateTimer();
		_canRender=false;
	}
	
	public void ResumeGame(){
		super.ResumeGame();
		ValidateUpdateTimer();
	}

	public void UpdateGame(){
		//
		//Ok, this isn't very polite - if keyboard enabled, we just thrash showSoftInput.
		//
		//But showSoftInput doesn't seem to be too reliable - esp. after onResume - and I haven't found a way to
		//determine if keyboard is showing, so what can yer do...
		//
		if( _keyboardEnabled ){
			InputMethodManager mgr=(InputMethodManager)_activity.getSystemService( Context.INPUT_METHOD_SERVICE );
			mgr.showSoftInput( _view,0 );		//0 is 'magic'! InputMethodManager.SHOW_IMPLICIT does weird things...
		}
		super.UpdateGame();
	}
	
	public void Run(){

		//touch input handling	
		SensorManager sensorManager=(SensorManager)_activity.getSystemService( Context.SENSOR_SERVICE );
		List<Sensor> sensorList=sensorManager.getSensorList( Sensor.TYPE_ACCELEROMETER );
		Iterator<Sensor> it=sensorList.iterator();
		if( it.hasNext() ){
			Sensor sensor=it.next();
			sensorManager.registerListener( this,sensor,SensorManager.SENSOR_DELAY_GAME );
		}
		
		//audio volume control
		_activity.setVolumeControlStream( AudioManager.STREAM_MUSIC );
		
		//GL version
		if( CerberusConfig.OPENGL_GLES20_ENABLED.equals( "1" ) ){
			//
			//_view.setEGLContextClientVersion( 2 );	//API 8 only!
			//
			try{
				Class clas=_view.getClass();
				Class parms[]=new Class[]{ Integer.TYPE };
				Method setVersion=clas.getMethod( "setEGLContextClientVersion",parms );
				Object args[]=new Object[1];
				args[0]=Integer.valueOf( 2 );
				setVersion.invoke( _view,args );
			}catch( Exception ex ){
			}
		}

		_view.setRenderer( this );
		_view.setFocusableInTouchMode( true );
		_view.requestFocus();
	}
	
	//***** GLSurfaceView.Renderer *****

	public void onDrawFrame( GL10 gl ){
		if( !_canRender ) return;
		
		StartGame();

		if( _updateRate==0 ){
			UpdateGame();
			RenderGame();
			return;
		}
			
		if( _nextUpdate==0 ){
			_nextUpdate=System.nanoTime();
		}else{
			long delay=_nextUpdate-System.nanoTime();
			if( delay>0 ){
				try{
					Thread.sleep( delay/1000000 );
				}catch( InterruptedException e ){
					_nextUpdate=0;
				}
			}
		}
			
		int i=0;
		for( ;i<4;++i ){

			UpdateGame();
			if( _nextUpdate==0 ) break;
			
			_nextUpdate+=_updatePeriod;
			if( _nextUpdate>System.nanoTime() ) break;
		}
		if( i==4 ) _nextUpdate=0;
		
		RenderGame();		
	}
	
	public void onSurfaceChanged( GL10 gl,int width,int height ){
	}
	
	public void onSurfaceCreated( GL10 gl,EGLConfig config ){
		_canRender=true;
		DiscardGraphics();
	}
	
	//***** SensorEventListener *****
	
	public void onAccuracyChanged( Sensor sensor,int accuracy ){
	}
	
	public void onSensorChanged( SensorEvent event ){
		Sensor sensor=event.sensor;
		float x,y,z;
		switch( sensor.getType() ){
		case Sensor.TYPE_ACCELEROMETER:
//			switch( _display.getRotation() ){
			switch( _display.getOrientation() ){	//deprecated in API 8, but we support 3...
			case Surface.ROTATION_0:
				x=event.values[0]/-9.81f;
				y=event.values[1]/9.81f;
				break;
			case Surface.ROTATION_90:
				x=event.values[1]/9.81f;
				y=event.values[0]/9.81f;
				break;
			case Surface.ROTATION_180:
				x=event.values[0]/9.81f;
				y=event.values[1]/-9.81f;
				break;
			case Surface.ROTATION_270:
				x=event.values[1]/-9.81f;
				y=event.values[0]/-9.81f;
				break;
			default:
				x=event.values[0]/-9.81f;
				y=event.values[1]/9.81f;
				break;
			}
			z=event.values[2]/-9.81f;
			MotionEvent( BBGameEvent.MotionAccel,-1,x,y,z );
			break;
    default:
      break;
		}
	}
}

class AndroidGame extends Activity{

	BBAndroidGame _game;
	
	GameView _view;
	
	//***** GameView *****

	public static class GameView extends BBAndroidGame.GameView{

		public GameView( Context context ){
			super( context );
		}
		
		public GameView( Context context,AttributeSet attrs ){
			super( context,attrs );
		}
	}
	
	//***** Activity *****
	public void onWindowFocusChanged( boolean hasFocus ){
		if( hasFocus ){
			_view.onResume();
			_game.ResumeGame();
		}else{
			_game.SuspendGame();
			_view.onPause();
		}
	}

	@Override
	public void onBackPressed(){
		//deprecating this!
		_game.KeyEvent( BBGameEvent.KeyDown,27 );
		_game.KeyEvent( BBGameEvent.KeyUp,27 );
		
		//new KEY_BACK value...
		_game.KeyEvent( BBGameEvent.KeyDown,0x1a0 );
		_game.KeyEvent( BBGameEvent.KeyUp,0x1a0 );
	}

	@Override
	public void onStart(){
		super.onResume();
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onStart();
		}
	}
	
	@Override
	public void onRestart(){
		super.onResume();
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onRestart();
		}
	}
	
	@Override
	public void onResume(){
		super.onResume();
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onResume();
		}
	}
	
	@Override 
	public void onPause(){
		super.onPause();
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onPause();
		}
	}

	@Override
	public void onStop(){
		super.onResume();
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onStop();
		}
	}
	
	@Override
	protected void onDestroy(){
		super.onDestroy();
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onDestroy();
		}
	}
	
	@Override
	protected void onActivityResult( int requestCode,int resultCode,Intent data ){
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onActivityResult( requestCode,resultCode,data );
		}
	}
	
	@Override
	public void onNewIntent( Intent intent ){
		super.onNewIntent( intent );
		for( ActivityDelegate delegate : _game._activityDelegates ){
			delegate.onNewIntent( intent );
		}
	}
}
