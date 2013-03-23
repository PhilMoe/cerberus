//
//  ImageViewController.h
//  CuteAnimals
//
//  Copyright 2012 Google, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "GAI.h"
#import "NavController.h"

@interface ImageViewController : GAITrackedViewController

@property(nonatomic, retain) NavController *navController;
@property(nonatomic, retain) UIImageView *imageView;

- (id)initWithNibName:(NSString *)nibNameOrNil
               bundle:(NSBundle *)nibBundleOrNil
                title:(NSString *)title
                image:(UIImage *)image;

@end
