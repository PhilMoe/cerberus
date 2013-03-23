//
//  GADCustomEventBannerDelegate.h
//  Google AdMob Ads SDK
//
//  Copyright 2012 Google Inc. All rights reserved.
//
// Call back to this delegate in your custom event. You must call
// customEventBanner:didReceiveAd: when there is an ad to show, or
// customEventBanner:didFailAd: when there is no ad to show. Otherwise, if
// enough time passed (several seconds) after the SDK called the
// requestBannerAd: method of your custom event, the mediation SDK will consider
// the request timed out, and move on to the next ad network.
//

#import <Foundation/Foundation.h>

@protocol GADCustomEventBanner;

@protocol GADCustomEventBannerDelegate <NSObject>

// Your Custom Event object must call this when it receives or creates an ad
// view.
- (void)customEventBanner:(id<GADCustomEventBanner>)customEvent
             didReceiveAd:(UIView *)view;

// Your Custom Event object must call this when it fails to receive or
// create the ad view. Pass along any error object sent from the ad network's
// SDK, or an NSError describing the error. Pass nil if not available.
- (void)customEventBanner:(id<GADCustomEventBanner>)customEvent
                didFailAd:(NSError *)error;

// Your Custom Event object should call this when the user touches or "clicks"
// the ad to initiate an action. When the SDK receives this callback, it reports
// the click back to the Mediation server. This callback is optional.
- (void)customEventBanner:(id<GADCustomEventBanner>)customEvent
        clickDidOccurInAd:(UIView *)view;

// The rootViewController that you set in GADBannerView.
// Use this UIViewController to show a modal view when a user taps on the ad.
@property (nonatomic, readonly)
    UIViewController *viewControllerForPresentingModalView;


// When you call the following methods, the call will be propagated back to the
// GADBannerViewDelegate that you implemented and passed to GADBannerView.

// Your Custom Event should call this when the user taps an ad and a modal view
// appears.
- (void)customEventBannerWillPresentModal:(id<GADCustomEventBanner>)customEvent;

// Your Custom Event should call this when the user dismisses the modal view
// and the modal view is about to go away.
- (void)customEventBannerWillDismissModal:(id<GADCustomEventBanner>)customEvent;

// Your Custom Event should call this when the user dismisses the modal view
// and the modal view has gone away.
- (void)customEventBannerDidDismissModal:(id<GADCustomEventBanner>)customEvent;

// Your Custom Event should call this method when a user action will result in
// App switching.
- (void)customEventBannerWillLeaveApplication:
    (id<GADCustomEventBanner>)customEvent;

@end
