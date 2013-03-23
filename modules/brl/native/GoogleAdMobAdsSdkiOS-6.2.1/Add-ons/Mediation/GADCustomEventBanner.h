//
//  GADCustomEventBanner.h
//  Google AdMob Ads SDK
//
//  Copyright 2012 Google Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "GADAdSize.h"
#import "GADCustomEventBannerDelegate.h"
#import "GADCustomEventRequest.h"

// The protocol for a Custom Event of the banner type.
// Your Custom Event handler object for banners must implement this protocol.
// The requestBannerAd method will be called when Mediation schedules your
// Custom Event to be executed.
@protocol GADCustomEventBanner <NSObject>

// This method is called by Mediation when your Custom Event is scheduled to
// be executed. Results of the execution should be reported back via the
// delegate. |adSize| is the size of the ad as configured in the Mediation UI
// for the Mediation Placement. |serverParameter| and |serverLabel| are the
// parameter and label configured in the Mediation UI for the Custom Event.
// |request| contains information about the ad request, some of those are from
// GADRequest.
- (void)requestBannerAd:(GADAdSize)adSize
              parameter:(NSString *)serverParameter
                  label:(NSString *)serverLabel
                request:(GADCustomEventRequest *)request;

// You should call back to the |delegate| with the results of the execution
// to ensure Mediation behaves correctly. The delegate is assigned, not
// retained, to prevent memory leak caused by circular retention.
//
// You can create accessor methods either by doing
//
// @synthesize delegate;
//
// in your class implementation, or define the methods -delegate and
// -setDelegate: yourself.
//
// In your object's -dealloc method, remember to nil out the delegate.
@property (nonatomic, assign) id<GADCustomEventBannerDelegate>delegate;

@end
