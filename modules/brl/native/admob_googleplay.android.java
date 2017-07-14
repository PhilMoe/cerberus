
import com.google.android.gms.ads.*;

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
		
		invalidateAdView();
	}
	
	public void HideAdView(){
		_adVisible=false;
		
		invalidateAdView();
	}
	
	public int AdViewWidth(){
		return (_adView!=null) ? _adView.getWidth() : 0;
	}
	
	public int AdViewHeight(){
		return (_adView!=null) ? _adView.getHeight() : 0 ;
	}

	private void addTestDevice( String test_dev,AdRequest.Builder builder ){
		if( test_dev.length()==0 ) return;
		if( test_dev.equals( "TEST_EMULATOR" ) ) test_dev=AdRequest.DEVICE_ID_EMULATOR;
		builder.addTestDevice( test_dev );
	}
	
	private void invalidateAdView(){
		if( _adValid ){
			_adValid=false;
			BBAndroidGame.AndroidGame().GetGameView().post( this );
		}
	}
	
	private void updateAdView(){
	
		_adValid=true;
	
		Activity activity=BBAndroidGame.AndroidGame().GetActivity();
		
		RelativeLayout parent=(RelativeLayout)activity.findViewById( R.id.mainLayout );
		
		if( _adView!=null ){
			parent.removeView( _adView );
			_adView.destroy();
			_adView=null;
		}
		
		if( !_adVisible ){
			return;
		}
		
		AdSize sz=AdSize.BANNER;
		switch( _adStyle ){
		case 2:sz=AdSize.SMART_BANNER;break;
		case 3:sz=AdSize.SMART_BANNER;break;
		}
		
		_adView=new AdView( activity );
		_adView.setAdSize( sz );
		_adView.setAdUnitId( CerberusConfig.ADMOB_PUBLISHER_ID );

		//weird voodoo to make adView appear instantly(ish). Without this, you have to wait about 60 seconds regardless of ad timeout setting.
		_adView.setBackgroundColor( Color.BLACK );
		
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
		
		AdRequest.Builder builder=new AdRequest.Builder();
		
		addTestDevice( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE1,builder );
		addTestDevice( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE2,builder );
		addTestDevice( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE3,builder );
		addTestDevice( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE4,builder );
		
		AdRequest req=builder.build();

		_adView.loadAd( req );
	}
	
	public void run(){
		updateAdView();
	}
	
}

class BBAdmobInterstitial implements Runnable {

	// the kind of "singleton"
	static BBAdmobInterstitial _admob;
	// the ad
	InterstitialAd interstitialAd;
	// ad Unit ID
	String adUnitId;
	
	boolean loaded;
	
	// creates an instance of the object and start the thread
	static public BBAdmobInterstitial GetAdmobInterstitial(String adUnitId){
		if( _admob==null ) _admob=new BBAdmobInterstitial();
		_admob.startAd(adUnitId);
		return _admob;
	}

	// displays the ad to the user if it is ready
	public void ShowAdViewInterstitial( ){
		if (interstitialAd != null ) {
			if( loaded ){
				loaded=false;
				BBAndroidGame.AndroidGame().GetGameView().post(this);
			}
		}
	}
	
	// start the thread 
	private void startAd(String adUnitId){
		this.adUnitId = adUnitId;
		BBAndroidGame.AndroidGame().GetGameView().post(this);
	}
	
	// loads an ad
	private void loadAd(){
		if (interstitialAd != null ) {
			AdRequest.Builder adRequest = new AdRequest.Builder();
			interstitialAd.loadAd(adRequest.build());
		}
	}
	
	// the runner
	public void run(){
	
		if( interstitialAd != null ){
			interstitialAd.show();
			return;
		}
		
		Activity activity = BBAndroidGame.AndroidGame().GetActivity();
		interstitialAd = new InterstitialAd( activity );
		interstitialAd.setAdUnitId(adUnitId);
		
		// set listener so we load a new ad when the user closes one
		interstitialAd.setAdListener(new AdListener() {
		
			public void onAdFailedToLoad(int errorCode) {
			}			
			
			public void onAdClosed() {
				loadAd();
			}
			
			public void onAdLeftApplication() {
			}
			
			public void onAdLoaded() {
				loaded=true;
			}
			
			public void onAdOpened() {
			}

		});
		
		// load the first ad
		loadAd();
	}
}