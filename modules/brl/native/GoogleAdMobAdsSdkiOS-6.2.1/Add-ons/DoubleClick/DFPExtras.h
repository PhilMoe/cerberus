//
//  DFPExtras.h
//  Google Ads iOS SDK
//
//  Copyright (c) 2012 Google Inc. All rights reserved.
//
//  To add DFP extras to an ad request:
//    DFPExtras *extras = [[[DFPExtras alloc] init] autorelease];
//    extras.additionalParameters =
//        [NSDictionary dictionaryWithObjectsAndKeys:
//          @"value", @"key",
//          nil];
//    GADRequest *request = [GADRequest request];
//    [request registerAdNetworkExtras:extras];
//

#import "GADAdMobExtras.h"

@interface DFPExtras : GADAdMobExtras

@property (nonatomic, copy) NSString *publisherProvidedID;

@end
