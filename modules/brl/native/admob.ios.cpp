
//admob.ios.h

#import "GADBannerView.h"

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
	
	BBMonkeyAppDelegate *appDelegate=(BBMonkeyAppDelegate*)[[UIApplication sharedApplication] delegate];
    
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
    
	[_view loadRequest:[GADRequest request]];
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
