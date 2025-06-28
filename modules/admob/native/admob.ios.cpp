//admob.ios.h

#import <GoogleMobileAds/GoogleMobileAds.h>
#import <UserMessagingPlatform/UserMessagingPlatform.h>
#import <UIKit/UIKit.h>

//Obj-C Forward Declarations of Delegates for all Ad Types (Banners, Interstitials, Rewarded Ads, ...)
@interface BannerDelegate : NSObject <GADBannerViewDelegate>
@property (nonatomic, copy) NSString *adUnitId;
@property(nonatomic, strong) GADBannerView *bannerView;
@property(nonatomic, assign) int width;
@property(nonatomic, assign) int height;
@property(nonatomic, assign) int position;
- (void)LoadBanner:(int)bannerPos;
- (void)removeBanner;
- (int)getBannerWidth;
- (int)getBannerHeight;
- (int)getBannerPosition;
- (bool)isBannerLoaded;
- (void)setBannerVisibility:(bool)isVisible;
@end


@interface InterstitialDelegate : NSObject <GADFullScreenContentDelegate>
@property (nonatomic, copy) NSString *adUnitId;
@property(nonatomic, strong) GADInterstitialAd *interstitial;
- (void)loadInterstitial;
- (void)showInterstitial;
- (bool)isInterstitialLoaded;
@end


@interface RewardedDelegate : NSObject <GADFullScreenContentDelegate>
@property (nonatomic, copy) NSString *adUnitId;
@property(nonatomic, strong) GADRewardedAd *rewardedAd;
@property(nonatomic, assign) bool _isRewardEarned;
@property (nonatomic, copy) NSString *rewardType;
@property(nonatomic, assign) int rewardAmount;
- (void)loadRewarded;
- (void)ShowRewarded;
- (bool)isRewardedLoaded;
- (bool)isRewardEarned;
- (NSString*)getRewardType;
- (int)getRewardAmount;
@end


class AdmobNative : public Object {
private:
	static bool isMobileAdsSdkInitialized;
	static int _debugGeographySetting;
	static Array<String> _testDeviceIds;
	
    static BannerDelegate* bannerDelegate;
    static InterstitialDelegate* interstitialDelegate;
    static RewardedDelegate* rewardedDelegate;
    
    static NSString *const defaultBannerAdUnitId;
    
    static NSString *const defaultInterstitialAdUnitId;
    
    static NSString *const defaultRewardedAdUnitId;

public:
    static CallbackSerializerNative* callbackSerializer;
    
	static const int EVENT_ADMOB_INITIALIZED = 16;
		
	static const int EVENT_BANNER_LOADED = 0;
	static const int EVENT_BANNER_FAILED = 1;
	static const int EVENT_BANNER_CLICKED = 2;
    
	static const int EVENT_INTERSTITIAL_LOADED = 3;
	static const int EVENT_INTERSTITIAL_FAILED = 4;
	static const int EVENT_INTERSTITIAL_SHOWN = 5;
	static const int EVENT_INTERSTITIAL_DISMISSED = 6;
	static const int EVENT_INTERSTITIAL_FAILED_TO_SHOW = 7;
	static const int EVENT_INTERSTITIAL_CLICKED = 8;

	static const int EVENT_REWARDED_LOADED = 9;
	static const int EVENT_REWARDED_FAILED = 10;
	static const int EVENT_REWARDED_SHOWN = 11;
	static const int EVENT_REWARDED_DISMISSED = 12;
	static const int EVENT_REWARDED_FAILED_TO_SHOW = 13;
	static const int EVENT_REWARDED_CLICKED = 14;
	static const int EVENT_REWARD_EARNED = 15;

	static const int DEBUG_GEOGRAPHY_DISABLED = UMPDebugGeographyDisabled;
	static const int DEBUG_GEOGRAPHY_EEA = UMPDebugGeographyEEA;
	static const int DEBUG_GEOGRAPHY_NOT_EEA = UMPDebugGeographyNotEEA;
	//static const int DEBUG_GEOGRAPHY_OTHER = UMPDebugGeographyOther;
	//static const int DEBUG_GEOGRAPHY_REGULATED_US_STATE = UMPDebugGeographyRegulatedUSState;
	
	static bool _bannerVisibility;

	static void SetDebugGeography(int debugGeographySetting=DEBUG_GEOGRAPHY_DISABLED) {
		_debugGeographySetting = debugGeographySetting;
	}


	static void SetTestDevices(Array<String> testDeviceIds=Array<String>()) {
		_testDeviceIds = testDeviceIds;
	}


	static void Initialize(String bannerAdUnitId="", String interstitialAdUnitId="", String rewardedAdUnitId="", CallbackSerializerNative* serializer=NULL) {  
        if (!bannerDelegate) {
			callbackSerializer = serializer;
            bannerDelegate = [[BannerDelegate alloc] init];
            if (bannerAdUnitId=="") {
	            [bannerDelegate setAdUnitId:defaultBannerAdUnitId];
            } else {
	            [bannerDelegate setAdUnitId:bannerAdUnitId.ToNSString()];
	        }
            
            interstitialDelegate = [[InterstitialDelegate alloc] init];
            if (interstitialAdUnitId=="") {
	            [interstitialDelegate setAdUnitId:defaultInterstitialAdUnitId];
            } else {
	            [interstitialDelegate setAdUnitId:interstitialAdUnitId.ToNSString()];
	        }
            
            rewardedDelegate = [[RewardedDelegate alloc] init];
            if (rewardedAdUnitId=="") {
	            [rewardedDelegate setAdUnitId:defaultRewardedAdUnitId];
            } else {
	            [rewardedDelegate setAdUnitId:rewardedAdUnitId.ToNSString()];
	        }
        	
			NSMutableArray<NSString *> *nsTestDeviceIds = [[NSMutableArray alloc] init];

			Array<String > tempArray = _testDeviceIds;
			int index=0;
			while(index<tempArray.Length()){
				String id=tempArray.At(index);
				index=index+1;
				[nsTestDeviceIds addObject:id.ToNSString()];
			}
			
			// Set testDevices to show test-ads there.
			GADMobileAds.sharedInstance.requestConfiguration.testDeviceIdentifiers = nsTestDeviceIds;
			
        	BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];        	
			UMPRequestParameters *parameters = [[UMPRequestParameters alloc] init];

			// For testing purposes, you can force a UMPDebugGeography of EEA or not EEA.
			UMPDebugSettings *debugSettings = [[UMPDebugSettings alloc] init];
			debugSettings.geography = _debugGeographySetting;
			debugSettings.testDeviceIdentifiers = nsTestDeviceIds; // Set testDevices to enable geography consent testing.
			parameters.debugSettings = debugSettings;
			
			// Requesting an update to consent information should be called on every app launch.
			[UMPConsentInformation.sharedInstance
				requestConsentInfoUpdateWithParameters:parameters
										completionHandler:^(NSError *_Nullable requestConsentError) {
											if (requestConsentError) {
												//consentGatheringComplete(requestConsentError);
											} else {
												[UMPConsentForm loadAndPresentIfRequiredFromViewController:appDelegate->viewController completionHandler:^(NSError *_Nullable loadAndPresentError) {
													// Consent has been gathered.
													//consentGatheringComplete(loadAndPresentError);
													if (UMPConsentInformation.sharedInstance.canRequestAds) {
                                    					startGoogleMobileAdsSDK();
                                  					}

												}];
											}
			                           }];
          
			// This sample attempts to load ads using consent obtained in the previous session.
			if (UMPConsentInformation.sharedInstance.canRequestAds) {
				startGoogleMobileAdsSDK();
			}
		}
	}


	static void ResetConsentStatus() {
		NSLog(@"ResetConsentStatus from Cpp");
		[UMPConsentInformation.sharedInstance reset];
	}


	static void ShowConsentDialog() {
		NSLog(@"ShowConsentDialog from Cpp");
		BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
		UIViewController *rootViewController = appDelegate->viewController;
		
		dispatch_async(dispatch_get_main_queue(), ^{
			[UMPConsentForm presentPrivacyOptionsFormFromViewController:rootViewController completionHandler:^(NSError *_Nullable formError) {
				if (formError) {
					UIAlertController *alertController = [UIAlertController alertControllerWithTitle:formError.localizedDescription message:@"Please try again later."
							preferredStyle:UIAlertControllerStyleAlert];
					UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action){}];
					[alertController addAction:defaultAction];
					[rootViewController presentViewController:alertController animated:YES completion:nil];
				}
			}];
		});
	}


	static bool IsConsentButtonMandatory() {
		NSLog(@"IsConsentButtonMandatory from Cpp");
		return UMPConsentInformation.sharedInstance.privacyOptionsRequirementStatus == UMPPrivacyOptionsRequirementStatusRequired;
	}


	static void startGoogleMobileAdsSDK() {
	  static dispatch_once_t onceToken;
	  dispatch_once(&onceToken, ^{
	    // Initialize the Google Mobile Ads SDK.
	    [GADMobileAds.sharedInstance startWithCompletionHandler:^(GADInitializationStatus * _Nonnull){
			isMobileAdsSdkInitialized = true;
			AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_ADMOB_INITIALIZED);
		}];
	  });
	}


	static bool IsInitialized() {
		NSLog(@"IsInitialized from Cpp");
		return isMobileAdsSdkInitialized;
	}


    static void LoadBanner(int position) {
		NSLog(@"LoadBanner from Cpp");
		[bannerDelegate loadBanner:position];
    }
    

    static void RemoveBanner() {
		NSLog(@"HideBanner from Cpp");
		[bannerDelegate removeBanner];
    }


    static int GetBannerWidth() {
		NSLog(@"GetBannerWidth from Cpp");
		CGFloat scale = [UIScreen mainScreen].scale;
		return [bannerDelegate getBannerWidth]*scale;
    }


    static int GetBannerHeight() {
		NSLog(@"GetBannerHeight from Cpp");
		CGFloat scale = [UIScreen mainScreen].scale;
		return [bannerDelegate getBannerHeight]*scale;
    }


    static int GetBannerSafeAreaX() {
		NSLog(@"GetSafeAreaX from Cpp");
		BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
		CGRect frame = UIEdgeInsetsInsetRect(appDelegate->viewController.view.frame, appDelegate->viewController.view.safeAreaInsets);
		CGFloat scale = [UIScreen mainScreen].scale;
		return frame.origin.x * scale;
    }


    static int GetBannerSafeAreaY() {
		NSLog(@"GetSafeAreaY from Cpp");
		BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
		CGRect frame = UIEdgeInsetsInsetRect(appDelegate->viewController.view.frame, appDelegate->viewController.view.safeAreaInsets);
		CGFloat scale = [UIScreen mainScreen].scale;
		if (GetBannerPosition()==1) {
			return frame.origin.y * scale + GetBannerHeight();
		} else {
			return frame.origin.y * scale;
		}
    }


    static int GetBannerSafeAreaWidth() {
		NSLog(@"GetSafeAreaWidth from Cpp");
		BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
		CGRect frame = UIEdgeInsetsInsetRect(appDelegate->viewController.view.frame, appDelegate->viewController.view.safeAreaInsets);
		CGFloat scale = [UIScreen mainScreen].scale;
		return frame.size.width * scale;
    }


    static int GetBannerSafeAreaHeight() {
		NSLog(@"GetSafeAreaHeight from Cpp");
		BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
		CGRect frame = UIEdgeInsetsInsetRect(appDelegate->viewController.view.frame, appDelegate->viewController.view.safeAreaInsets);
		CGFloat scale = [UIScreen mainScreen].scale;
		return frame.size.height * scale - GetBannerHeight();
    }
    

    static int GetBannerPosition() {
		NSLog(@"GetBannerPosition from Cpp");
		return [bannerDelegate getBannerPosition];
    }
    

    static bool IsBannerLoaded() {
		NSLog(@"IsBannerLoaded from Cpp");
		return [bannerDelegate isBannerLoaded];
    }

    static void SetBannerVisibility(bool isVisible) {
        NSLog(@"SetBannerVisibility from Cpp");
        _bannerVisibility = isVisible;
        [bannerDelegate setBannerVisibility:isVisible];
    }

    static void LoadInterstitial() {
		NSLog(@"LoadInterstitial from Cpp");
		[interstitialDelegate loadInterstitial];
    }


    static void ShowInterstitial() {
		NSLog(@"ShowInterstitial from Cpp");
		[interstitialDelegate showInterstitial];
    }
    

    static bool IsInterstitialLoaded() {
		NSLog(@"IsInterstitialLoaded from Cpp");
		return [interstitialDelegate isInterstitialLoaded];
    }


    static void LoadRewarded() {
		NSLog(@"LoadRewarded from Cpp");
		[rewardedDelegate loadRewarded];
    }


    static void ShowRewarded() {
		NSLog(@"ShowRewarded from Cpp");
		[rewardedDelegate showRewarded];
    }
    

    static bool IsRewardedLoaded() {
		NSLog(@"IsRewardedLoaded from Cpp");
		return [rewardedDelegate isRewardedLoaded];
    }
    
    
    static bool IsRewardEarned() {
		NSLog(@"IsRewardEarned from Cpp");
		if (rewardedDelegate==nil) return false;
		return [rewardedDelegate isRewardEarned];
    }
    
    static String GetRewardType() {
		NSLog(@"GetRewardType from Cpp");
		return String([rewardedDelegate getRewardType]);
    }


    static int GetRewardAmount() {
		NSLog(@"GetRewardAmount from Cpp");
		return [rewardedDelegate getRewardAmount];
    }
};

int AdmobNative::_debugGeographySetting = AdmobNative::DEBUG_GEOGRAPHY_DISABLED;
Array<String> AdmobNative::_testDeviceIds = Array<String>();
bool AdmobNative::isMobileAdsSdkInitialized = false;
CallbackSerializerNative* AdmobNative::callbackSerializer = NULL;

BannerDelegate* AdmobNative::bannerDelegate = nil;
NSString *const AdmobNative::defaultBannerAdUnitId 		= @"ca-app-pub-3940256099942544/2435281174";
bool AdmobNative::_bannerVisibility = false;

InterstitialDelegate* AdmobNative::interstitialDelegate = nil;
NSString *const AdmobNative::defaultInterstitialAdUnitId 	= @"ca-app-pub-3940256099942544/4411468910";

RewardedDelegate* AdmobNative::rewardedDelegate = nil;
NSString *const AdmobNative::defaultRewardedAdUnitId 		= @"ca-app-pub-3940256099942544/1712485313";


@implementation BannerDelegate

- (void)loadBanner:(int)bannerPos {
	NSLog(@"loadBanner called");
	if (self.bannerView) [self removeBanner];
	
	NSLayoutAttribute layoutAttribute = NSLayoutAttributeBottom;
	
	if (bannerPos==0) {
		layoutAttribute = NSLayoutAttributeBottom;
	} else if (bannerPos==1) {
		layoutAttribute = NSLayoutAttributeTop;
	}
	
	self.position = bannerPos;
	
	BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
	
	CGRect frame = UIEdgeInsetsInsetRect(appDelegate->viewController.view.frame, appDelegate->viewController.view.safeAreaInsets);
	CGFloat viewWidth = frame.size.width;

	// Here the current interface orientation is used. If the ad is being preloaded
	// for a future orientation change or different orientation, the function for the
	// relevant orientation should be used.
	GADAdSize adaptiveSize = GADCurrentOrientationAnchoredAdaptiveBannerAdSizeWithWidth(viewWidth);
	
	CGSize bannerSize = CGSizeFromGADAdSize(adaptiveSize);
	self.width = bannerSize.width;
	self.height = bannerSize.height;

	// In this case, we instantiate the banner with desired ad size.
	self.bannerView = [[GADBannerView alloc] initWithAdSize:adaptiveSize];
	self.bannerView.delegate = self;

	self.bannerView.translatesAutoresizingMaskIntoConstraints = NO;
		[appDelegate->viewController.view addSubview:self.bannerView];
		// This example doesn't give width or height constraints, as the provided
		// ad size gives the banner an intrinsic content size to size the view.
		[appDelegate->viewController.view addConstraints:@[
				[NSLayoutConstraint constraintWithItem:self.bannerView
									attribute:layoutAttribute
									relatedBy:NSLayoutRelationEqual
									toItem:appDelegate->viewController.view.safeAreaLayoutGuide
									attribute:layoutAttribute
									multiplier:1
									constant:0],
				[NSLayoutConstraint constraintWithItem:self.bannerView
									attribute:NSLayoutAttributeCenterX
									relatedBy:NSLayoutRelationEqual
									toItem:appDelegate->viewController.view
									attribute:NSLayoutAttributeCenterX
									multiplier:1
									constant:0]
		]];	


	// Set the ad unit ID and view controller that contains the GADBannerView.
	self.bannerView.adUnitID = self.adUnitId;
	self.bannerView.rootViewController = appDelegate->viewController;
	[self.bannerView loadRequest:[GADRequest request]];
	
}

- (void)bannerViewDidReceiveAd:(GADBannerView *)bannerView {
    NSLog(@"Banner loaded successfully");
    [self setBannerVisibility:AdmobNative::_bannerVisibility];
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_BANNER_LOADED);
}

- (void)bannerView:(GADBannerView *)bannerView didFailToReceiveAdWithError:(NSError *)error {
    NSLog(@"Banner failed to load with error: %@", [error localizedDescription]);
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_BANNER_FAILED);
}

- (void)bannerViewDidRecordClick:(GADBannerView *)bannerView {
    NSLog(@"Banner was clicked");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_BANNER_CLICKED);
}

- (void)removeBanner {
	NSLog(@"removeBanner called");
	if( !self.bannerView ) return;
	[self.bannerView removeFromSuperview];
	[self.bannerView release];
	self.bannerView = nil;
}

- (int)getBannerWidth {
	NSLog(@"getBannerWidth called");
	return self.bannerView ? self.bannerView.bounds.size.width : 0;
}

- (int)getBannerHeight {
	NSLog(@"getBannerHeight called");
	return self.bannerView ? self.bannerView.bounds.size.height : 0;
}

- (int)getBannerPosition {
	NSLog(@"getBannerPosition called");
	return self.position;
}

- (bool)isBannerLoaded {
	NSLog(@"isBannerLoaded called");
	return self.bannerView!=nil;
}

- (void)setBannerVisibility:(bool)isVisible {
    NSLog(@"setBannerVisibility called");
    if (!self.bannerView) return;
    if (isVisible) {
        [self.bannerView setHidden:NO];
    } else {
		[self.bannerView setHidden:YES];
    }
}
@end


@implementation InterstitialDelegate

- (void)loadInterstitial {
    GADRequest *request = [GADRequest request];
    [GADInterstitialAd loadWithAdUnitID:self.adUnitId
                              request:request
                    completionHandler:^(GADInterstitialAd *ad, NSError *error) {
        if (error) {
        	self.interstitial = nil;
            NSLog(@"Failed to load interstitial ad with error: %@", [error localizedDescription]);
            AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_INTERSTITIAL_FAILED);
            return;
        }
        self.interstitial = ad;
        self.interstitial.fullScreenContentDelegate = self;
        AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_INTERSTITIAL_LOADED);
    }];
}

- (void)showInterstitial {
    NSLog(@"showInterstitial called");
    if (self.interstitial) {
        NSLog(@"Schowinnngggnngngngn");
        [self.interstitial presentFromRootViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    } else {
    	self.interstitial = nil;
        NSLog(@"Ad wasn't ready - loading started now");
        AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_INTERSTITIAL_FAILED_TO_SHOW);
    }
}

- (bool)isInterstitialLoaded {
    NSLog(@"isInterstitialLoaded called");
    return self.interstitial!=nil;
}

- (void)adDidDismissFullScreenContent:(GADInterstitialAd *)ad {
    self.interstitial = nil;
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_INTERSTITIAL_DISMISSED);
    //[self loadInterstitial];
}

- (void)ad:(nonnull id<GADFullScreenPresentingAd>)ad
    didFailToPresentFullScreenContentWithError:(nonnull NSError *)error {
    NSLog(@"Ad did fail to present full screen content.");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_INTERSTITIAL_FAILED_TO_SHOW);
}

- (void)adWillPresentFullScreenContent:(nonnull id<GADFullScreenPresentingAd>)ad {
    NSLog(@"Ad will present full screen content.");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_INTERSTITIAL_SHOWN);
}

- (void)adDidRecordClick:(nonnull id<GADFullScreenPresentingAd>)ad {
    NSLog(@"Ad was clicked.");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_INTERSTITIAL_CLICKED);
}

@end


@implementation RewardedDelegate

- (void)loadRewarded {
  GADRequest *request = [GADRequest request];
  [GADRewardedAd
      loadWithAdUnitID:self.adUnitId
                request:request
      completionHandler:^(GADRewardedAd *ad, NSError *error) {
        if (error) {
        	self.rewardedAd = nil;
          NSLog(@"Rewarded ad failed to load with error: %@", [error localizedDescription]);
          AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARDED_FAILED);
          return;
        }
        self.rewardedAd = ad;
        NSLog(@"Rewarded ad loaded.");
        self.rewardedAd.fullScreenContentDelegate = self;
        AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARDED_LOADED);
      }];
}

- (void)showRewarded {
  if (self.rewardedAd) {
    // The UIViewController parameter is nullable.
    [self.rewardedAd presentFromRootViewController:nil userDidEarnRewardHandler:^{
        GADAdReward *reward = self.rewardedAd.adReward;
        NSLog(@"Reward: %d %@", reward.amount, reward.type);
        
        // Reward the user!
        self._isRewardEarned = true;
        self.rewardType = reward.type;
        self.rewardAmount = [reward.amount intValue];
        AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARD_EARNED);
     }];
  } else {
  	self.rewardedAd = nil;
    NSLog(@"Ad wasn't ready");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARDED_FAILED_TO_SHOW);
  }
}

- (bool)isRewardedLoaded {
    NSLog(@"isRewardedLoaded called");
    return self.rewardedAd!=nil;
}

- (bool)isRewardEarned {
    NSLog(@"isRewardEarned called");
    return self._isRewardEarned;
}

- (NSString*)getRewardType {
    NSLog(@"getRewardType called");
    return self.rewardType;
}

- (int)getRewardAmount {
    NSLog(@"getRewardAmount called");
    return self.rewardAmount;
}

- (void)ad:(nonnull id<GADFullScreenPresentingAd>)ad
    didFailToPresentFullScreenContentWithError:(nonnull NSError *)error {
    self.rewardedAd = nil;
    NSLog(@"Ad did fail to present full screen content.");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARDED_FAILED_TO_SHOW);
}

- (void)adWillPresentFullScreenContent:(nonnull id<GADFullScreenPresentingAd>)ad {
    NSLog(@"Ad will present full screen content.");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARDED_SHOWN);
    
    self._isRewardEarned = false;
    self.rewardType = @"";
    self.rewardAmount = 0;   
}

- (void)adDidDismissFullScreenContent:(nonnull id<GADFullScreenPresentingAd>)ad {
	self.rewardedAd = nil;
    NSLog(@"Ad did dismiss full screen content.");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARDED_DISMISSED);
    //[self loadRewarded];
}

- (void)adDidRecordClick:(nonnull id<GADFullScreenPresentingAd>)ad {
    NSLog(@"Ad was clicked.");
    AdmobNative::callbackSerializer->AddCallback(AdmobNative::EVENT_REWARDED_CLICKED);
}

@end
