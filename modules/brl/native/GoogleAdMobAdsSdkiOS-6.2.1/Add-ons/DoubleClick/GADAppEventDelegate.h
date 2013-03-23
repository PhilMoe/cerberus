//
//  GADAppEventDelegate.h
//  Google Ads iOS SDK
//
//  Copyright (c) 2012 Google Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "GADBannerView.h"
#import "GADInterstitial.h"

@protocol GADAppEventDelegate <NSObject>

// Implement your app event within these methods. The delegate will be notified
// when the SDK receives an app event message from the ad.
@optional

- (void)adView:(GADBannerView *)banner
    didReceiveAppEvent:(NSString *)name
              withInfo:(NSString *)info;

- (void)interstitial:(GADInterstitial *)interstitial
    didReceiveAppEvent:(NSString *)name
              withInfo:(NSString *)info;

@end
