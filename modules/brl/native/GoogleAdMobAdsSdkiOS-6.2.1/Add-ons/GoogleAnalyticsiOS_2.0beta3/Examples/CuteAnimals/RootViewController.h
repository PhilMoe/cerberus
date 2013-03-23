//
//  RootViewController.h
//  CuteAnimals
//
//  Copyright 2012 Google, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RootViewController :
    GAITrackedViewController<UITableViewDelegate, UITableViewDataSource>

@property(nonatomic, retain) NavController *navController;
@property(nonatomic, retain) IBOutlet UITableView *tableView;

@end
