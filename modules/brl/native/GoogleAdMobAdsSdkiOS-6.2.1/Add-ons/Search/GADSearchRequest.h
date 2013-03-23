//
//  GADSearchRequest.h
//  Google Search Ads iOS SDK
//
//  Copyright 2011 Google Inc. All rights reserved.
//

#import "GADRequest.h"
#import <UIKit/UIKit.h>

// Types of borders for search ads.
typedef enum {
  kGADSearchBorderTypeNone,
  kGADSearchBorderTypeDashed,
  kGADSearchBorderTypeDotted,
  kGADSearchBorderTypeSolid
} GADSearchBorderType;

// Specifies parameters and controls for search ads.
@interface GADSearchRequest : NSObject

@property (nonatomic, copy) NSString *query;
@property (nonatomic, readonly) UIColor *backgroundColor;
@property (nonatomic, readonly) UIColor *gradientFrom;
@property (nonatomic, readonly) UIColor *gradientTo;
@property (nonatomic, retain) UIColor *headerColor;
@property (nonatomic, retain) UIColor *descriptionTextColor;
@property (nonatomic, retain) UIColor *anchorTextColor;
@property (nonatomic, copy) NSString *fontFamily;
@property (nonatomic) int headerTextSize;
@property (nonatomic, retain) UIColor *borderColor;
@property (nonatomic) GADSearchBorderType borderType;
@property (nonatomic) int borderThickness;
@property (nonatomic, copy) NSString *customChannels;

// The request object used to request ad. Pass the value returned by the method
// to GADSearchBannerView to get the ad in the format specified.
- (GADRequest *)request;

// A solid background color for rendering the ad. The background of the ad
// can either be a solid color, or a gradient, which can be specified through
// setBackgroundGradientFrom:toColor: method. If both solid and gradient
// background is requested, only the latter is considered.
- (void)setBackgroundSolid:(UIColor *)color;

// A linear gradient background color for rendering the ad. The background of
// the ad can either be a linear gradient, or a solid color, which can be
// specified through setBackgroundSolid method. If both solid and gradient
// background is requested, only the latter is considered.
- (void)setBackgroundGradientFrom:(UIColor *)from toColor:(UIColor *)toColor;

@end
