//
//  GADCustomEventRequest.h
//  Google AdMob Ads SDK
//
//  Copyright 2012 Google Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "GADRequest.h"

@class GADCustomEventExtras;

@interface GADCustomEventRequest : NSObject

// The end user's gender set in GADRequest. If none specified,
// returns kGADGenderUnknown.
@property (nonatomic, readonly) GADGender userGender;

// The end user's birthday set in GADRequest. If none specified, returns nil.
@property (nonatomic, readonly) NSDate *userBirthday;

// The end user's latitude, longitude and accuracy, set in GADRequest. If
// none specified, hasLocation retuns NO, and userLatitude, userLongitude
// and userLocationAccuracyInMeters will all return 0.
@property (nonatomic, readonly) BOOL userHasLocation;
@property (nonatomic, readonly) CGFloat userLatitude;
@property (nonatomic, readonly) CGFloat userLongitude;
@property (nonatomic, readonly) CGFloat userLocationAccuracyInMeters;

// Description of the user's location, in free form text, set in GADRequest.
// If not available, returns nil. This may be set even if userHasLocation
// is NO.
@property (nonatomic, readonly) NSString *userLocationDescription;

// Keywords set in GADRequest. If none, returns nil.
@property (nonatomic, readonly) NSArray *userKeywords;

// The additional parameters set by the app in GADRequest.h. This allows you
// to pass additional information from your app to your Custom Event object. To
// do so, create an instance of GADCustomEventExtras to pass to GADRequest
// -registerAdNetworkExtras:. The instance should have an NSDictionary set for a
// particular custom event label. That NSDictionary becomes the
// additionalParameters here.
@property (nonatomic, readonly) NSDictionary *additionalParameters;

// Whether you have set the testing property in GADRequest.
@property (nonatomic, readonly) BOOL isTesting;

@end
