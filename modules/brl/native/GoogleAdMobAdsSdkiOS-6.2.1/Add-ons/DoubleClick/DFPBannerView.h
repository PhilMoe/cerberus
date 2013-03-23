//
//  DFPBannerView.h
//  Google AdMob Ads iOS SDK
//
//  Copyright (c) 2012 Google Inc. All rights reserved.
//

#import "GADBannerView.h"

@protocol GADAdSizeDelegate;
@protocol GADAppEventDelegate;

@interface DFPBannerView : GADBannerView

// Optional delegate object that will be notified if a creative sends
// app events. Remember to nil out this property before releasing the object
// that implements the GADAppEventDelegate protocol to avoid crashing the app.
@property (nonatomic, assign) NSObject<GADAppEventDelegate> *appEventDelegate;

// Optional delegate object that will be notified if a creative causes the
// banner to change size. Remember to nil out this property before releasing the
// object that implements the GADAdSizeDelegate protocol to avoid crashing the
// app.
@property (nonatomic, assign) NSObject<GADAdSizeDelegate> *adSizeDelegate;

// Optional array of GADAdSize to specify all valid sizes that are appropriate
// for this slot. Never create your own GADAdSize directly. Use one of the
// predefined standard ad sizes (such as kGADAdSizeBanner), or create one using
// the GADAdSizeFromCGSize method.
//
// Example code:
//   GADAdSize size1 = GADAdSizeFromCGSize(CGSizeMake(320, 50));
//   GADAdSize size2 = GADAdSizeFromCGSize(CGSizeMake(300, 50));
//   NSMutableArray *validSizes = [NSMutableArray array];
//   [validSizes addObject:[NSValue valueWithBytes:&size1
//       objCType:@encode(GADAdSize)]];
//   [validSizes addObject:[NSValue valueWithBytes:&size2
//       objCType:@encode(GADAdSize)]];
//
//   myView.validAdSizes = validSizes;
@property (nonatomic, retain) NSArray *validAdSizes;

@end
