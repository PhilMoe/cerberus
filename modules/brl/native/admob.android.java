
import com.google.ads.*;

class BBAdmob implements Runnable{

	static BBAdmob _admob;
	
	int _adStyle;
	int _adLayout;
	boolean _adVisible;

	AdView _adView;
	boolean _adValid=true;
	
	static public BBAdmob GetAdmob(){
		if( _admob==null ) _admob=new BBAdmob();
		return _admob;
	}
	
	public void ShowAdView( int style,int layout ){
		_adStyle=style;
		_adLayout=layout;
		_adVisible=true;
		
		if( _adValid ){
			_adValid=false;
			BBAndroidGame.AndroidGame().GetGameView().post( this );
		}
	}
	
	public void HideAdView(){
		_adVisible=false;
		
		if( _adValid ){
			_adValid=false;
			BBAndroidGame.AndroidGame().GetGameView().post( this );
		}
	}
	
	public int AdViewWidth(){
		return (_adView!=null) ? _adView.getWidth() : 0;
	}
	
	public int AdViewHeight(){
		return (_adView!=null) ? _adView.getHeight() : 0 ;
	}
	
	private static void AddTestDev( String test_dev,AdRequest req ){
		if( test_dev.length()==0 ) return;
		if( test_dev.equals( "TEST_EMULATOR" ) ) test_dev=AdRequest.TEST_EMULATOR;
		req.addTestDevice( test_dev );
	}
	
	public void run(){
	
		_adValid=true;
		
		Activity activity=BBAndroidGame.AndroidGame().GetActivity();
		
		RelativeLayout parent=(RelativeLayout)activity.findViewById( R.id.mainLayout );
		
		if( _adView!=null ){
			parent.removeView( _adView );
			_adView.destroy();
			_adView=null;
		}
		
		if( !_adVisible ) return;
		
		AdSize sz=AdSize.BANNER;
		switch( _adStyle ){
		case 2:sz=AdSize.SMART_BANNER;break;
		case 3:sz=AdSize.SMART_BANNER;break;
		}
		
		_adView=new AdView( activity,sz,CerberusConfig.ADMOB_PUBLISHER_ID );
				
		RelativeLayout.LayoutParams params=new RelativeLayout.LayoutParams( RelativeLayout.LayoutParams.WRAP_CONTENT,RelativeLayout.LayoutParams.WRAP_CONTENT );
		
		int rule1=RelativeLayout.CENTER_HORIZONTAL,rule2=RelativeLayout.CENTER_VERTICAL;
		
		switch( _adLayout ){
		case 1:rule1=RelativeLayout.ALIGN_PARENT_TOP;rule2=RelativeLayout.ALIGN_PARENT_LEFT;break;
		case 2:rule1=RelativeLayout.ALIGN_PARENT_TOP;rule2=RelativeLayout.CENTER_HORIZONTAL;break;
		case 3:rule1=RelativeLayout.ALIGN_PARENT_TOP;rule2=RelativeLayout.ALIGN_PARENT_RIGHT;break;
		case 4:rule1=RelativeLayout.ALIGN_PARENT_BOTTOM;rule2=RelativeLayout.ALIGN_PARENT_LEFT;break;
		case 5:rule1=RelativeLayout.ALIGN_PARENT_BOTTOM;rule2=RelativeLayout.CENTER_HORIZONTAL;break;
		case 6:rule1=RelativeLayout.ALIGN_PARENT_BOTTOM;rule2=RelativeLayout.ALIGN_PARENT_RIGHT;break;
		}
		
		params.addRule( rule1 );
		params.addRule( rule2 );
		
		parent.addView( _adView,params );

		AdRequest req=new AdRequest();
		
		AddTestDev( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE1,req );
		AddTestDev( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE2,req );
		AddTestDev( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE3,req );
		AddTestDev( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE4,req );
		
		_adView.loadAd( req );
	}
}
