
//import com.google.android.gms.ads.*;
import androidx.annotation.NonNull;
import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;
import com.google.android.gms.ads.RequestConfiguration;
import android.util.Log;


final class Variables
{
    static boolean __bb__MobileAds_init=false;
}

class BBAdmob implements Runnable{

	static BBAdmob _admob;
	
	int _adStyle;
	int _adLayout;
	boolean _adVisible;

	AdView _adView;
	AdRequest.Builder _builder;
	boolean _adValid=true;
	private static final String TAG = "[Cerberus]";

	static Activity _activity;
	
	static public BBAdmob GetAdmob(){
		Log.d(TAG, "BBAdmob GetAdmob()");
		if( _admob==null ) _admob=new BBAdmob();
		_activity = BBAndroidGame.AndroidGame().GetActivity();
		if( Variables.__bb__MobileAds_init == false ) {
			Log.d(TAG, "BBAdmob MobileAds.initialize");
		    MobileAds.initialize(_activity, new OnInitializationCompleteListener() {
		        @Override
		        public void onInitializationComplete(InitializationStatus initializationStatus) {}
		    });
		    Variables.__bb__MobileAds_init = true ;
	    }
		return _admob;
	}
	
	public void ShowAdView( int style,int layout ){
		Log.d(TAG, "BBAdmob ShowAdView()");
		_adStyle=style;
		_adLayout=layout;
		_adVisible=true;
		
		invalidateAdView();
	}
	
	public void HideAdView(){
		Log.d(TAG, "BBAdmob HideAdView()");
		_adVisible=false;
		
		invalidateAdView();
	}
	
	public int AdViewWidth(){
		return (_adView!=null) ? _adView.getWidth() : 0;
	}
	
	public int AdViewHeight(){
		return (_adView!=null) ? _adView.getHeight() : 0 ;
	}

	private void invalidateAdView(){
		if( _adValid ){
			Log.d(TAG, "BBAdmob invalidateAdView()");
			_adValid=false;
			BBAndroidGame.AndroidGame().GetGameView().post( this );
		}
	}
	
	private void updateAdView(){
		Log.d(TAG, "BBAdmob updateAdView()");
	
		_adValid=true;
	
		RelativeLayout parent=(RelativeLayout)_activity.findViewById( R.id.mainLayout );
		
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
		
		_adView=new AdView( _activity );
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
		
		_builder = new AdRequest.Builder();
		
	
		List<String> testDevices = new ArrayList<>();
		testDevices.add(AdRequest.DEVICE_ID_EMULATOR);
		
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE1.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE1);
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE2.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE2);
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE3.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE3);
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE4.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE4);
		
		RequestConfiguration requestConfiguration
   			 = new RequestConfiguration.Builder()
        			.setTestDeviceIds(testDevices)
        			.build();
		MobileAds.setRequestConfiguration(requestConfiguration);
		
		AdRequest req=_builder.build();

		_adView.loadAd( req );
	}
	
	public void run(){
		Log.d(TAG, "BBAdmob run()");
		updateAdView();
	}
	
}

class BBAdmobInterstitial implements Runnable {

	// the kind of "singleton"
	static BBAdmobInterstitial _admob;
	// the ad
	//AdRequest.Builder _builder;
	//InterstitialAd _interstitialAd;
	// ad Unit ID
	String adUnitId;
	
	boolean loaded;
	
	private InterstitialAd mInterstitialAd;
	private static final String TAG = "[Cerberus]";
	static Activity _activity;
	
	// creates an instance of the object and start the thread
	static public BBAdmobInterstitial GetAdmobInterstitial(String adUnitId){
		//Toast.makeText(MyActivity.this, "GetAdmobInterstitial", Toast.LENGTH_SHORT).show();
		Log.d(TAG, "BBAdmobInterstitial GetAdmobInterstitial()");

		if( _admob==null ) _admob=new BBAdmobInterstitial();
		_activity = BBAndroidGame.AndroidGame().GetActivity();

		if( Variables.__bb__MobileAds_init == false ) {
			Log.d(TAG, "BBAdmobInterstitial MobileAds.initialize");
		    MobileAds.initialize(_activity, new OnInitializationCompleteListener() {
		        @Override
		        public void onInitializationComplete(InitializationStatus initializationStatus) {}
		    });
		    Variables.__bb__MobileAds_init = true ;
	    }

		_admob.startAd(adUnitId);
		return _admob;
	}

	// displays the ad to the user if it is ready
	public void ShowAdViewInterstitial( ){
		if (mInterstitialAd != null ) {
			if( loaded ){
				Log.d(TAG, "BBAdmobInterstitial ShowAdViewInterstitial() -> loaded = true");
				loaded=false;
				BBAndroidGame.AndroidGame().GetGameView().post(this);
			} else {	
				Log.e(TAG, "BBAdmobInterstitial ShowAdViewInterstitial() -> loaded = false");
			}
		} else {	
			Log.e(TAG, "BBAdmobInterstitial ShowAdViewInterstitial() -> mInterstitialAd = null");	
		}
	}
	
	// start the thread 
	private void startAd(String adUnitId){
		Log.d(TAG, "BBAdmobInterstitial startAd()");
		this.adUnitId = adUnitId;
		BBAndroidGame.AndroidGame().GetGameView().post(this);
	}
	
	// loads an ad
	private void loadAd(){
		Log.d(TAG, "BBAdmobInterstitial loadAd started ************");	

		List<String> testDevices = new ArrayList<>();
		testDevices.add(AdRequest.DEVICE_ID_EMULATOR);
		
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE1.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE1);
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE2.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE2);
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE3.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE3);
		if( CerberusConfig.ADMOB_ANDROID_TEST_DEVICE4.length()!=0 ) testDevices.add(CerberusConfig.ADMOB_ANDROID_TEST_DEVICE4);
		
		RequestConfiguration requestConfiguration
   			 = new RequestConfiguration.Builder()
        			.setTestDeviceIds(testDevices)
        			.build();
		MobileAds.setRequestConfiguration(requestConfiguration);

		AdRequest adRequest = new AdRequest.Builder().build();

      	InterstitialAd.load(_activity, this.adUnitId, adRequest, new InterstitialAdLoadCallback() {
	      
	      @Override
	      public void onAdLoaded(@NonNull InterstitialAd interstitialAd) {
	        // The mInterstitialAd reference will be null until
	        // an ad is loaded.
	        mInterstitialAd = interstitialAd;
	        loaded=true;
	       Log.d(TAG, "BBAdmobInterstitial onAdLoaded");
	       
		   interstitialAd.setFullScreenContentCallback(new FullScreenContentCallback(){
			  @Override
			  public void onAdDismissedFullScreenContent() {
			    // Called when fullscreen content is dismissed.
			    //mInterstitialAd = null;
			    Log.d(TAG, "BBAdmobInterstitial FullScreenContentCallback -> The ad was dismissed.");
			  }
			
			  @Override
			  public void onAdFailedToShowFullScreenContent(AdError adError) {
			    // Called when fullscreen content failed to show.
			    Log.d(TAG, "BBAdmobInterstitial FullScreenContentCallback -> The ad failed to show.");
			  }
			
			  @Override
			  public void onAdShowedFullScreenContent() {
			    // Called when fullscreen content is shown.
			    // Make sure to set your reference to null so you don't
			    // show it a second time.
			    mInterstitialAd = null;
			    Log.d(TAG, "BBAdmobInterstitial FullScreenContentCallback -> The ad was shown.");
			    loadAd();
			  }
			});	       
	       
	      }

	      @Override
	      public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) {
	       	Log.e(TAG, "BBAdmobInterstitial onAdFailedToLoad");
	        // Handle the error
	        Log.e(TAG, loadAdError.getMessage());
	        mInterstitialAd = null;
	      }
    	});
    	
	}
	
	// the runner
	public void run(){
	
		if( mInterstitialAd != null ){
			Log.d(TAG, "BBAdmobInterstitial run() -> mInterstitialAd.show");
			mInterstitialAd.show(_activity);
			return;
		} else {	
			Log.e(TAG, "BBAdmobInterstitial run() -> mInterstitialAd = null");	
		}
		
		// load the first ad
		loadAd();
	}
}