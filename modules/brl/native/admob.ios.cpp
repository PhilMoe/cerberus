
//admob.ios.h

#import <GoogleMobileAds/GoogleMobileAds.h>

class BBAdmob{

	static BBAdmob *_admob;
	
	GADBannerView *_view;

public:
	BBAdmob();
	
	static BBAdmob *GetAdmob();
	
	void ShowAdView( int style,int layout );
	void HideAdView();
	int  AdViewWidth();
	int  AdViewHeight();
};

//admob.ios.cpp

#define _QUOTE(X) #X
#define _STRINGIZE(X) _QUOTE(X)

BBAdmob *BBAdmob::_admob;

BBAdmob::BBAdmob():_view(0){
}

BBAdmob *BBAdmob::GetAdmob(){
	if( !_admob ) _admob=new BBAdmob();
	return _admob;
}

void BBAdmob::ShowAdView( int style,int layout ){

	if( _view ){
		[_view removeFromSuperview];
		[_view release];
	}
	
	GADAdSize sz=kGADAdSizeBanner;
	switch( style ){
	case 2:sz=kGADAdSizeSmartBannerPortrait;break;
	case 3:sz=kGADAdSizeSmartBannerLandscape;break;
	}
	
	_view=[[GADBannerView alloc] initWithAdSize:sz];
	if( !_view ) return;
    
	_view.adUnitID=@_STRINGIZE(CFG_ADMOB_PUBLISHER_ID);
	
	BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
    
	_view.rootViewController=appDelegate->viewController;
	
	UIView *appView=appDelegate->view;

	CGRect b1=appView.bounds;
	CGRect b2=_view.bounds;

	switch( layout ){
	case 1:
		_view.autoresizingMask=UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleBottomMargin;
		break;
	case 2:
		b2.origin.x=(b1.size.width-b2.size.width)/2;
		_view.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleBottomMargin;
		break;
	case 3:
		b2.origin.x=(b1.size.width-b2.size.width);
		_view.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleBottomMargin;
		break;
	case 4:
		b2.origin.y=(b1.size.height-b2.size.height);
		_view.autoresizingMask=UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleTopMargin;
		break;
	case 5:
		b2.origin.x=(b1.size.width-b2.size.width)/2;
		b2.origin.y=(b1.size.height-b2.size.height);
		_view.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleTopMargin;
		break;
	case 6:
		b2.origin.x=(b1.size.width-b2.size.width);
		b2.origin.y=(b1.size.height-b2.size.height);
		_view.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleTopMargin;
		break;
	default:
		b2.origin.x=(b1.size.width-b2.size.width)/2;
		b2.origin.y=(b1.size.height-b2.size.height)/2;
		_view.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin|UIViewAutoresizingFlexibleTopMargin|UIViewAutoresizingFlexibleBottomMargin;
	}

	_view.frame=b2;
		    
	[appView addSubview:_view];
	
	GADRequest *request=[GADRequest request];
   
	[_view loadRequest:request];
}

void BBAdmob::HideAdView(){
	if( !_view ) return;
	
	[_view removeFromSuperview];
	
	[_view release];
	
	_view=0;
}

int BBAdmob::AdViewWidth(){
	return _view ? _view.bounds.size.width : 0;
}

int BBAdmob::AdViewHeight(){
	return _view ? _view.bounds.size.height : 0;
}

class BBAdmobInterstitial
{

  static BBAdmobInterstitial *_admobInterstitial;

  GADInterstitial *_interstitialAd;
  NSString *adUnitId;

  void loadAd();

public:
  BBAdmobInterstitial();

  static BBAdmobInterstitial *GetAdmobInterstitial( String adUnitID );

  void ShowAdViewInterstitial();
};

BBAdmobInterstitial *BBAdmobInterstitial::_admobInterstitial;

BBAdmobInterstitial::BBAdmobInterstitial(): _interstitialAd(0)
{
}

BBAdmobInterstitial *BBAdmobInterstitial::GetAdmobInterstitial( String adUnitId )
{
  if( !_admobInterstitial ) _admobInterstitial = new BBAdmobInterstitial();
  _admobInterstitial->adUnitId = adUnitId.ToNSString();
  _admobInterstitial->loadAd();
  return _admobInterstitial;
}

void BBAdmobInterstitial::loadAd()
{
  _interstitialAd = [[GADInterstitial alloc] init];

  if( _interstitialAd )
  {
    _interstitialAd.adUnitID = adUnitId;
    [_interstitialAd loadRequest:[GADRequest request]];
  }
}

void BBAdmobInterstitial::ShowAdViewInterstitial()
{
  if( _interstitialAd )
  {
    if( _interstitialAd.isReady )
    {
      BBCerberusAppDelegate *appDelegate=(BBCerberusAppDelegate*)[[UIApplication sharedApplication] delegate];
      UIViewController *rootViewController = appDelegate->viewController;
      [_interstitialAd presentFromRootViewController:rootViewController];

      // load the next ad
      _admobInterstitial->loadAd();
    }
  }
}