//
//  GADCustomEventInterstitialDelegate.h
//  Google AdMob Ads SDK
//
//  Copyright 2012 Google Inc. All rights reserved.
//
// Call back to this delegate in your custom event. You must call
// customEventInterstitial:didReceiveAd: when there is an ad to show, or
// customEventInterstitial:didFailAd: when there is no ad to show. Otherwise, if
// enough time passed (several seconds) after the SDK called the
// requestInterstitialAdWithParameter: method of your custom event, the
// mediation SDK will consider the request timed out, and move on to the next ad
// network.
//

#import <Foundation/Foundation.h>

@protocol GADCustomEventInterstitial;

@protocol GADCustomEventInterstitialDelegate <NSObject>

// Your Custom Event object must call this when it receives or creates an
// interstitial ad. If there is an ad object, pass it in the method call.
// Pass nil if the ad object is not available.
- (void)customEventInterstitial:(id<GADCustomEventInterstitial>)customEvent
                   didReceiveAd:(NSObject *)ad;

// Your Custom Event object must call this when it fails to receive or
// create the ad. Pass along any error object sent from the ad network's
// SDK, or an NSError describing the error. Pass nil if not available.
- (void)customEventInterstitial:(id<GADCustomEventInterstitial>)customEvent
                      didFailAd:(NSError *)error;


// When you call any of the the following methods, the call will be propagated
// back to the GADInterstitialDelegate that you implemented and passed to
// GADInterstitial.

// Your Custom Event should call this when the interstitial is being displayed.
- (void)customEventInterstitialWillPresent:
    (id<GADCustomEventInterstitial>)customEvent;

// Your Custom Event should call this when the interstitial is about to be
// dismissed.
- (void)customEventInterstitialWillDismiss:
    (id<GADCustomEventInterstitial>)customEvent;

// Your Custom Event should call this when the interstitial has been dismissed.
- (void)customEventInterstitialDidDismiss:
    (id<GADCustomEventInterstitial>)customEvent;

// Your Custom Event should call this method when a user action will result in
// App switching.
- (void)customEventInterstitialWillLeaveApplication:
    (id<GADCustomEventInterstitial>)customEvent;

@end
