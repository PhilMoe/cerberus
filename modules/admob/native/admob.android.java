import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;

import androidx.annotation.NonNull;
import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.OnUserEarnedRewardListener;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;
import com.google.android.gms.ads.rewarded.RewardItem;
import com.google.android.gms.ads.rewarded.RewardedAd;
import com.google.android.gms.ads.rewarded.RewardedAdLoadCallback;
import java.util.concurrent.atomic.AtomicBoolean;

//For UMP consent management
import com.google.android.ump.ConsentInformation;
import com.google.android.ump.ConsentInformation.OnConsentInfoUpdateSuccessListener;
import com.google.android.ump.ConsentInformation.OnConsentInfoUpdateFailureListener;
import com.google.android.ump.ConsentRequestParameters;
import com.google.android.ump.FormError;
import com.google.android.ump.UserMessagingPlatform;

import com.google.android.ump.ConsentDebugSettings;
import com.google.android.ump.ConsentForm.OnConsentFormDismissedListener;
import com.google.android.ump.ConsentInformation.PrivacyOptionsRequirementStatus;

//For Banner Ads
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.RequestConfiguration;
import android.view.WindowMetrics;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;

//For Interstitial Ads
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;

//For Rewarded Ads
import com.google.android.gms.ads.rewarded.RewardedAd;

//For screen insets handling (systemBars, screenCutouts)
//import androidx.core.view.OnApplyWindowInsetsListener;
import androidx.core.graphics.Insets;
import android.view.WindowInsets.Type

class AdmobNative {
	
	private static final String TAG = "AdmobNative";
	
	public static final int DEBUG_GEOGRAPHY_DISABLED = ConsentDebugSettings.DebugGeography.DEBUG_GEOGRAPHY_DISABLED;
	public static final int DEBUG_GEOGRAPHY_EEA = ConsentDebugSettings.DebugGeography.DEBUG_GEOGRAPHY_EEA;
	public static final int DEBUG_GEOGRAPHY_NOT_EEA = ConsentDebugSettings.DebugGeography.DEBUG_GEOGRAPHY_NOT_EEA;
	//public static final int DEBUG_GEOGRAPHY_OTHER = ConsentDebugSettings.DebugGeography.DEBUG_GEOGRAPHY_OTHER;
	//public static final int DEBUG_GEOGRAPHY_REGULATED_US_STATE = ConsentDebugSettings.DebugGeography.DEBUG_GEOGRAPHY_REGULATED_US_STATE;

	public static final int EVENT_ADMOB_INITIALIZED = 16;
	
	public static final int EVENT_BANNER_LOADED = 0;
	public static final int EVENT_BANNER_FAILED = 1;
	public static final int EVENT_BANNER_CLICKED = 2;
	
	public static final int EVENT_INTERSTITIAL_LOADED = 3;
	public static final int EVENT_INTERSTITIAL_FAILED = 4;
	public static final int EVENT_INTERSTITIAL_SHOWN = 5;
	public static final int EVENT_INTERSTITIAL_DISMISSED = 6;
	public static final int EVENT_INTERSTITIAL_FAILED_TO_SHOW = 7;
	public static final int EVENT_INTERSTITIAL_CLICKED = 8;
	
	public static final int EVENT_REWARDED_LOADED = 9;
	public static final int EVENT_REWARDED_FAILED = 10;
	public static final int EVENT_REWARDED_SHOWN = 11;
	public static final int EVENT_REWARDED_DISMISSED = 12;
	public static final int EVENT_REWARDED_FAILED_TO_SHOW = 13;
	public static final int EVENT_REWARDED_CLICKED = 14;
	public static final int EVENT_REWARD_EARNED = 15;

	private static FragmentActivity _activity;
	
	// Use an atomic boolean to initialize the Google Mobile Ads SDK and load ads once.
	private static final AtomicBoolean isMobileAdsInitializeCalled = new AtomicBoolean(false);
	private static ConsentInformation _consentInformation;
	private static boolean _isMobileAdsSdkInitialized = false;
	private static int _debugGeographySetting = DEBUG_GEOGRAPHY_DISABLED;
	private static String[] _testDeviceIds = new String[0];

	private static CallbackSerializerNative _callbackSerializer;
	
	private static AdView _bannerAdView;
	private static int _bannerPosition;
	private static boolean _bannerVisibility = false;
	private static int _safeAreaX;
	private static int _safeAreaY;
	private static int _safeAreaWidth;
	private static int _safeAreaHeight;
	
	private static InterstitialAd _interstitialAd;
	private static boolean _isInterstitialAdLoading;
	
	private static RewardedAd _rewardedAd;
	private static boolean _isRewardEarned = false;
	private static String _rewardType = "";
	private static int _rewardAmount = 0;

	private static String _bannerAdUnitId = "ca-app-pub-3940256099942544/9214589741";
	private static String _interstitialAdUnitId = "ca-app-pub-3940256099942544/1033173712";
	private static String _rewardedAdUnitId = "ca-app-pub-3940256099942544/5224354917";
	

	public static void SetDebugGeography(int debugGeographySetting) {
		_debugGeographySetting = debugGeographySetting;
	}


	public static void SetTestDevices(String[] testDeviceIds) {
		_testDeviceIds = testDeviceIds;
	}	


	public static void Initialize(String bannerAdUnitId, String interstitialAdUnitId, String rewardedAdUnitId, CallbackSerializerNative serializer) {
		Log.d(TAG,"Initialize called!");
		_callbackSerializer = serializer;
		
		if (bannerAdUnitId!="") _bannerAdUnitId = bannerAdUnitId;
		if (interstitialAdUnitId!="") _interstitialAdUnitId = interstitialAdUnitId;
		if (rewardedAdUnitId!="") _rewardedAdUnitId = rewardedAdUnitId;
		
		_activity = BBAndroidGame.AndroidGame().GetActivity();
		
		_activity.runOnUiThread(new Runnable() {

			public void run() {			
				_InitializeUMP();	
			}
		});
	}
	
	
	private static void _InitializeUMP() {
		Log.d(TAG,"_InitializeUMP called!");
		
		ConsentDebugSettings.Builder debugSettingsBuilder = new ConsentDebugSettings.Builder(_activity);
		
		debugSettingsBuilder.setDebugGeography(_debugGeographySetting);
		// Check your logcat output for the hashed device ID e.g.
		// "Use new ConsentDebugSettings.Builder().addTestDeviceHashedId("ABCDEF012345")" to use
		// the debug functionality.
		for (String id : _testDeviceIds) {
			debugSettingsBuilder.addTestDeviceHashedId(id);
		}
		ConsentDebugSettings debugSettings = debugSettingsBuilder.build();

		ConsentRequestParameters params = new ConsentRequestParameters.Builder()
		.setConsentDebugSettings(debugSettings)
		.build();
		// Todo: Check if current device is testdevice now and show a toast in either case.
		
		// Also set the test devices for the ads configuration
		RequestConfiguration config = new RequestConfiguration.Builder()
		.setTestDeviceIds(Arrays.asList(_testDeviceIds))
		.build();
		MobileAds.setRequestConfiguration(config);

		_consentInformation = UserMessagingPlatform.getConsentInformation(_activity);
		_consentInformation.requestConsentInfoUpdate(
			_activity,
			params,
			(OnConsentInfoUpdateSuccessListener) () -> {
				UserMessagingPlatform.loadAndShowConsentFormIfRequired(
					_activity,
					(OnConsentFormDismissedListener) loadAndShowError -> {
						if (loadAndShowError != null) {
							// Consent gathering failed.
							Log.w(TAG, String.format("%s: %s",
								loadAndShowError.getErrorCode(),
								loadAndShowError.getMessage()));
						}

						// Consent has been gathered.
						if (_consentInformation.canRequestAds()) {
							Log.d(TAG,"canRequestAds()=True!");
							initializeMobileAdsSdk();
						}
					}
				);
			},
			(OnConsentInfoUpdateFailureListener) requestConsentError -> {
				// Consent gathering failed.
				Log.w(TAG, String.format("%s: %s",
					requestConsentError.getErrorCode(),
					requestConsentError.getMessage()));
			});
		
		// Check if you can initialize the Google Mobile Ads SDK in parallel
		// while checking for new consent information. Consent obtained in
		// the previous session can be used to request ads.
		if (_consentInformation.canRequestAds()) {
			Log.d(TAG,"canRequestAds()=True!");
			initializeMobileAdsSdk();
		}
	}
	
	
	private static void initializeMobileAdsSdk() {
		if (isMobileAdsInitializeCalled.getAndSet(true)) {
			return;
		}
		Log.d(TAG,"initializeMobileAdsSdk called!");
		
		new Thread(
		() -> {
			// Initialize the Google Mobile Ads SDK on a background thread.
			MobileAds.initialize(_activity);

			_activity.runOnUiThread(new Runnable() {
				public void run() {	
					if (_callbackSerializer != null) {
						_callbackSerializer.AddCallback(EVENT_ADMOB_INITIALIZED);
					}
					_isMobileAdsSdkInitialized=true;
				}
			});
			
		}).start();
		
	}
	
	
	public static boolean IsInitialized() {
		return _isMobileAdsSdkInitialized;
	}

	
	public static void ResetConsentStatus() {
		Log.d(TAG,"ResetConsentStatus called!");
		
		_activity.runOnUiThread(new Runnable() {
			public void run() {	
				if (_consentInformation==null) return;		
				_consentInformation.reset();	
			}
		});
	}
	
	
	public static boolean IsConsentButtonMandatory() {
		if (_consentInformation==null) return false;
		return _consentInformation.getPrivacyOptionsRequirementStatus() == PrivacyOptionsRequirementStatus.REQUIRED;
	}


	public static void ShowConsentDialog() {
		Log.d(TAG,"ShowConsentDialog called!");
		
		_activity.runOnUiThread(new Runnable() {
			public void run() {			
				UserMessagingPlatform.showPrivacyOptionsForm(
					_activity,
					formError -> {
						if (formError != null) {
							Log.e(TAG, formError.toString());
						}
					}
				);
			}
		});
	}

	
	public static void LoadBanner(int position) {
		Log.d(TAG,"LoadBanner called!");
		
		_activity.runOnUiThread(new Runnable() {
			public void run() {						
				_LoadBanner(position);
			}
		});
	}
	
	
	public static void _LoadBanner(int position) {
		_bannerPosition = position;
		if (_bannerAdView!=null) {
			_RemoveBanner();
		}
		// Create a new AdView.
		_bannerAdView = new AdView(_activity);
		_bannerAdView.setAdUnitId(_bannerAdUnitId);
		_bannerAdView.setAdSize(getAdSizeForAdaptiveBanner());
		_bannerAdView.setAdListener(new AdListener() {
			@Override
			public void onAdLoaded() {
				Log.d(TAG, "Banner loaded successfully");
				SetBannerVisibility(_bannerVisibility);
				if (_callbackSerializer != null) {
					_callbackSerializer.AddCallback(EVENT_BANNER_LOADED);
				}
			}

			@Override
			public void onAdFailedToLoad(LoadAdError loadAdError) {
				Log.e(TAG, "Banner failed to load with error: " + loadAdError.toString());
				if (_callbackSerializer != null) {
					_callbackSerializer.AddCallback(EVENT_BANNER_FAILED);
				}
			}

			@Override
			public void onAdClicked() {
				Log.d(TAG, "Banner was clicked");
				if (_callbackSerializer != null) {
					_callbackSerializer.AddCallback(EVENT_BANNER_CLICKED);
				}
			}
		});
	
		// Replace ad container with new ad view.
		RelativeLayout parent=(RelativeLayout)_activity.findViewById( R.id.mainLayout );
		//parent.removeAllViews(); //This also removes the game view .. bad!
		RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
			RelativeLayout.LayoutParams.MATCH_PARENT,
			RelativeLayout.LayoutParams.WRAP_CONTENT
		);

		WindowInsetsCompat insets = ViewCompat.getRootWindowInsets(parent);
		if (insets != null) {
			Insets systemInsets = insets.getInsetsIgnoringVisibility(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout());
			if (position == 0) {
				params.bottomMargin = systemInsets.bottom;
				params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
			} else {
				params.topMargin = systemInsets.top;
				params.addRule(RelativeLayout.ALIGN_PARENT_TOP);
			}
		}

		parent.addView(_bannerAdView, params);
	
		// Start loading the ad in the background.
		AdRequest adRequest = new AdRequest.Builder().build();
		_bannerAdView.loadAd(adRequest);
		
	}

	public static void SetBannerVisibility(boolean isVisible) {
		Log.d(TAG,"SetBannerVisibility called!");
		_bannerVisibility = isVisible;
		if (_bannerAdView==null) return;
		if (isVisible) {
			_activity.runOnUiThread(new Runnable() {
				public void run() {										
					_bannerAdView.setVisibility(View.VISIBLE);
					_bannerAdView.resume();
				}
			});
		} else {
			_activity.runOnUiThread(new Runnable() {
				public void run() {						
					_bannerAdView.pause();
					_bannerAdView.setVisibility(View.INVISIBLE);
				}
			});
		}
	}


	public static void RemoveBanner() {
		Log.d(TAG,"RemoveBanner called!");
		if (_bannerAdView==null) return;
		
		_activity.runOnUiThread(new Runnable() {
			public void run() {		
				_RemoveBanner();
			}
		});
	}

	private static void _RemoveBanner() {
		if (_bannerAdView==null) return;
		
		// Remove banner from view hierarchy.
		if (_bannerAdView.getParent() instanceof ViewGroup) {
			((ViewGroup) _bannerAdView.getParent()).removeView(_bannerAdView);
		}
		// Destroy the banner ad resources.
		_bannerAdView.destroy();
		// Drop reference to the banner ad.
		_bannerAdView = null;
	}


	public static boolean IsBannerLoaded(){
		return _bannerAdView!=null;
	}
	
	
	public static int GetBannerWidth(){
		return (_bannerAdView!=null) ? _bannerAdView.getWidth() : 0;
	}
	
	
	public static int GetBannerHeight(){
		return (_bannerAdView!=null) ? _bannerAdView.getHeight() : 0 ;
	}

	
	public static int GetBannerSafeAreaX(){
		View rootView = _activity.findViewById(android.R.id.content);
				
		RelativeLayout parent=(RelativeLayout)_activity.findViewById( R.id.mainLayout );        
		WindowInsetsCompat windowInsets = ViewCompat.getRootWindowInsets(parent);
		if (windowInsets != null) {
			Insets systemInsets = windowInsets.getInsetsIgnoringVisibility(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout());
			int top = systemInsets.top;
			int bottom = systemInsets.bottom;
			int left = systemInsets.left;
			int right = systemInsets.right;

			return left;
		}
		
		return 0;
	}


	public static int GetBannerSafeAreaY(){
		View rootView = _activity.findViewById(android.R.id.content);

		// Nur wenn View bereits im Layout ist
		
		int bannerHeight = 0;
		if (_bannerAdView!=null) {
			bannerHeight = _bannerAdView.getHeight();
		}

		RelativeLayout parent=(RelativeLayout)_activity.findViewById( R.id.mainLayout );        
		WindowInsetsCompat windowInsets = ViewCompat.getRootWindowInsets(parent);
		if (windowInsets != null) {
			Insets systemInsets = windowInsets.getInsetsIgnoringVisibility(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout());
			int top = systemInsets.top;
			int bottom = systemInsets.bottom;
			int left = systemInsets.left;
			int right = systemInsets.right;

			if (_bannerPosition==1) top+=bannerHeight;
			return top;
		}
		return 0;
	}


	public static int GetBannerSafeAreaWidth(){
		View rootView = _activity.findViewById(android.R.id.content);

		// Nur wenn View bereits im Layout ist
		
		RelativeLayout parent=(RelativeLayout)_activity.findViewById( R.id.mainLayout );        
		WindowInsetsCompat windowInsets = ViewCompat.getRootWindowInsets(parent);
		if (windowInsets != null) {
			Insets systemInsets = windowInsets.getInsetsIgnoringVisibility(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout());
			int top = systemInsets.top;
			int bottom = systemInsets.bottom;
			int left = systemInsets.left;
			int right = systemInsets.right;

			return rootView.getWidth();
		}
		return rootView.getWidth();
	}


	public static int GetBannerSafeAreaHeight(){
		View rootView = _activity.findViewById(android.R.id.content);

		// Nur wenn View bereits im Layout ist
		
		int bannerHeight = 0;
		if (_bannerAdView!=null) {
			bannerHeight = _bannerAdView.getHeight();
		}
		
		RelativeLayout parent=(RelativeLayout)_activity.findViewById( R.id.mainLayout );        
		WindowInsetsCompat windowInsets = ViewCompat.getRootWindowInsets(parent);
		if (windowInsets != null) {
			Insets systemInsets = windowInsets.getInsetsIgnoringVisibility(WindowInsetsCompat.Type.systemBars() | WindowInsetsCompat.Type.displayCutout());
			int top = systemInsets.top;
			int bottom = systemInsets.bottom;
			int left = systemInsets.left;
			int right = systemInsets.right;

			int usableHeight = rootView.getHeight() - top - bottom - bannerHeight;

			return usableHeight;
		}
		return rootView.getHeight();
	}


	// Get the AdSize from screen width.
	private static AdSize getAdSizeForAdaptiveBanner() {
		
		DisplayMetrics displayMetrics = _activity.getResources().getDisplayMetrics();
		int adWidthPixels = displayMetrics.widthPixels;
		
		if (VERSION.SDK_INT >= VERSION_CODES.R) {
			WindowMetrics windowMetrics = _activity.getWindowManager().getCurrentWindowMetrics();
			adWidthPixels = windowMetrics.getBounds().width();
		}
		
		float density = displayMetrics.density;
		int adWidth = (int) (adWidthPixels / density);
		return AdSize.getCurrentOrientationAnchoredAdaptiveBannerAdSize(_activity, adWidth);
	}


	public static void LoadInterstitial() {
		Log.d(TAG,"LoadInterstitial called!");
		_activity.runOnUiThread(new Runnable() {
			public void run() {						
				_LoadInterstitial();
			}
		});

		Log.d(TAG,"RequestInterstitialAd");
	}


	public static void _LoadInterstitial() {
		// Request a new ad if one isn't already loaded or loading.
		if (_isInterstitialAdLoading || _interstitialAd != null) {
			return;
		}
		
		_isInterstitialAdLoading = true;
		AdRequest adRequest = new AdRequest.Builder().build();
		InterstitialAd.load(
		_activity,
		_interstitialAdUnitId,
		adRequest,
		new InterstitialAdLoadCallback() {
			@Override
			public void onAdLoaded(@NonNull InterstitialAd interstitialAd) {
				_interstitialAd = interstitialAd;
				_isInterstitialAdLoading = false;
				Log.i(TAG, "onAdLoaded");
				if (_callbackSerializer != null) {
					_callbackSerializer.AddCallback(EVENT_INTERSTITIAL_LOADED);
				}
				
				_interstitialAd.setFullScreenContentCallback(
					new FullScreenContentCallback() {
						@Override
						public void onAdDismissedFullScreenContent() {
							_interstitialAd = null;
							Log.d("TAG", "The ad was dismissed.");
							if (_callbackSerializer != null) {
								_callbackSerializer.AddCallback(EVENT_INTERSTITIAL_DISMISSED);
							}
							//_LoadInterstitial();
						}
		
						@Override
						public void onAdFailedToShowFullScreenContent(AdError adError) {
							_interstitialAd = null;
							Log.d("TAG", "The ad failed to show.");
							if (_callbackSerializer != null) {
								_callbackSerializer.AddCallback(EVENT_INTERSTITIAL_FAILED_TO_SHOW);
							}
						}
		
						@Override
						public void onAdShowedFullScreenContent() {
							Log.d("TAG", "The ad was shown.");
							if (_callbackSerializer != null) {
								_callbackSerializer.AddCallback(EVENT_INTERSTITIAL_SHOWN);
							}
						}
					});
			}
		
			@Override
			public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) {
				Log.i(TAG, loadAdError.getMessage());
				_interstitialAd = null;
				_isInterstitialAdLoading = false;
				if (_callbackSerializer != null) {
					_callbackSerializer.AddCallback(EVENT_INTERSTITIAL_FAILED);
				}
			}
		});
	}

	public static void ShowInterstitial() {
		Log.d(TAG,"ShowInterstitial called!");
		
		_activity.runOnUiThread(new Runnable() {
			public void run() {										
				if (_interstitialAd != null) {
					_interstitialAd.show(_activity);
				}
			}
		});
	}

	
	public static boolean IsInterstitialLoaded(){
		return _interstitialAd!=null;
	}
	
	
	public static void LoadRewarded() {
		Log.d(TAG,"LoadRewarded called!");
		
		_activity.runOnUiThread(new Runnable() {
			public void run() {		
				AdRequest adRequest = new AdRequest.Builder().build();
				RewardedAd.load(_activity, _rewardedAdUnitId,
					adRequest, new RewardedAdLoadCallback() {
						@Override
						public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) {
							Log.e(TAG, loadAdError.toString());
							_rewardedAd = null;
							if (_callbackSerializer != null) {
								_callbackSerializer.AddCallback(EVENT_REWARDED_FAILED);
							}
						}

						@Override
						public void onAdLoaded(@NonNull RewardedAd ad) {
							_rewardedAd = ad;
							Log.d(TAG, "RewardedAd was loaded.");
							if (_callbackSerializer != null) {
								_callbackSerializer.AddCallback(EVENT_REWARDED_LOADED);
							}
							_rewardedAd.setFullScreenContentCallback(new FullScreenContentCallback() {
								@Override
								public void onAdClicked() {
									Log.d(TAG, "RewardedAd was clicked.");
								}
							
								@Override
								public void onAdDismissedFullScreenContent() {
									Log.d(TAG, "RewardedAd dismissed fullscreen content.");
									_rewardedAd = null;
									if (_callbackSerializer != null) {
										_callbackSerializer.AddCallback(EVENT_REWARDED_DISMISSED);
									}
									//LoadRewarded();
								}
							
								@Override
								public void onAdFailedToShowFullScreenContent(AdError adError) {
									Log.e(TAG, "RewardedAd failed to show fullscreen content.");
									_rewardedAd = null;
									if (_callbackSerializer != null) {
										_callbackSerializer.AddCallback(EVENT_REWARDED_FAILED_TO_SHOW);
									}
								}
							
								@Override
								public void onAdImpression() {
									Log.d(TAG, "RewardedAd recorded an impression.");
									_isRewardEarned = false;
									_rewardType = "";
									_rewardAmount = 0;
								}
							
								@Override
								public void onAdShowedFullScreenContent() {
									Log.d(TAG, "RewardedAd showed fullscreen content.");
									if (_callbackSerializer != null) {
										_callbackSerializer.AddCallback(EVENT_REWARDED_SHOWN);
									}
								}
							});						
						}		
				});
			}
		});

		Log.d(TAG," Requested Rewarded Ad");
	}
	
	public static void ShowRewarded() {
		Log.d(TAG,"ShowRewarded called!");
		
		_activity.runOnUiThread(new Runnable() {
			public void run() {																				
				if (_rewardedAd != null) {
					_rewardedAd.show(_activity, new OnUserEarnedRewardListener() {
						@Override
						public void onUserEarnedReward(@NonNull RewardItem rewardItem) {
							Log.d(TAG, "onUserEarnedReward called");
							_isRewardEarned = true;
							_rewardType = rewardItem.getType();
							_rewardAmount = rewardItem.getAmount();
							if (_callbackSerializer != null) {
								_callbackSerializer.AddCallback(EVENT_REWARD_EARNED);
							}
						}
					});
				} else {
					Log.d(TAG, "The rewarded ad wasn't ready yet.");
				}	
			}					
		});
	}
	
	
	public static boolean IsRewardedLoaded(){
		return _rewardedAd!=null;
	}	


	public static boolean IsRewardEarned(){
		return _isRewardEarned;
	}	


	public static String GetRewardType(){
		return _rewardType;
	}	
	

	public static int GetRewardAmount(){
		return _rewardAmount;
	}	
	
}




