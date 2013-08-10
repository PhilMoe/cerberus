
//gamecenter.h
//
#import <GameKit/GameKit.h>

@interface BBGameCenterDelegate : NSObject{
}

@end

class BBGameCenter{

	static BBGameCenter *_gameCenter;

	int _state;
	
	BBGameCenterDelegate *_delegate;

public:
	BBGameCenter();
	
	static BBGameCenter *GetGameCenter();
	
	bool GameCenterAvail();
	
	void StartGameCenter();
	int  GameCenterState();
	
	void ShowLeaderboard( String leaderboard_ID );
	void ReportScore( int score,String leaderboard_ID );
	
	void ShowAchievements();
    void ReportAchievement( float percent,String achievement_ID );
    
    //INTERNAL
    void GameCenterViewControllerDidFinish( UIViewController *vc );
};

//gamecenter.cpp

BBGameCenter *BBGameCenter::_gameCenter;

@implementation BBGameCenterDelegate

-(void)gameCenterViewControllerDidFinish:(GKGameCenterViewController*)vc{
	BBGameCenter::GetGameCenter()->GameCenterViewControllerDidFinish( vc );
}

-(void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController*)vc{
	BBGameCenter::GetGameCenter()->GameCenterViewControllerDidFinish( vc );
}

- (void)achievementViewControllerDidFinish:(GKAchievementViewController *)vc{
	BBGameCenter::GetGameCenter()->GameCenterViewControllerDidFinish( vc );
}

@end

BBGameCenter::BBGameCenter():_state(-1),_delegate(0){
	if( !GameCenterAvail() ) return;
	_delegate=[[BBGameCenterDelegate alloc] init];
	_state=0;
}

BBGameCenter *BBGameCenter::GetGameCenter(){
	if( !_gameCenter ) _gameCenter=new BBGameCenter();
	return _gameCenter;
}

bool BBGameCenter::GameCenterAvail(){

	// Check for presence of GKLocalPlayer API.
	Class gcClass=NSClassFromString( @"GKLocalPlayer" );

	// The device must be running running iOS 4.1 or later.
	NSString *reqSysVer=@"4.1";
	NSString *currSysVer=[[UIDevice currentDevice] systemVersion];
	BOOL osVersionSupported=([currSysVer compare:reqSysVer options:NSNumericSearch]!=NSOrderedAscending);

    return (gcClass && osVersionSupported);
}

void BBGameCenter::StartGameCenter(){

	if( _state ) return;
	
	GKLocalPlayer *localPlayer=[GKLocalPlayer localPlayer];
	
	if( localPlayer ){
		_state=1;
	    [localPlayer authenticateWithCompletionHandler:^(NSError *error){
			if( localPlayer.isAuthenticated ){
				_state=2;
			}else{
				_state=-1;
			}
	     }];
	}else{
		_state=-1;
	}
}

int BBGameCenter::GameCenterState(){

	return _state;
}

void BBGameCenter::ShowLeaderboard( String leaderboard_ID ){

	if( _state!=2 ) return;
	
	GKLeaderboardViewController *vc=[[GKLeaderboardViewController alloc] init];
	if( !vc ) return;
	
	vc.leaderboardDelegate=(id)_delegate;
	vc.timeScope=GKLeaderboardTimeScopeToday;
	vc.category=leaderboard_ID.ToNSString();
	
	_state=3;

	// Keep it ios<6 friendly for now...	
	// [BBIosGame::IosGame()->GetUIAppDelegate()->viewController presentViewController:vc animated:YES completion:nil];
	[BBIosGame::IosGame()->GetUIAppDelegate()->viewController presentModalViewController:vc animated:YES];
}	
    
void BBGameCenter::ReportScore( int value,String leaderboard_ID ){
    
	GKScore *score=[[GKScore alloc] initWithCategory:leaderboard_ID.ToNSString()];
	    		
	score.value=value;
	score.context=0;
	    		
	[score reportScoreWithCompletionHandler:^(NSError *error){} ];
}

void BBGameCenter::ShowAchievements(){

	if( _state!=2 ) return;

	GKAchievementViewController *vc=[[GKAchievementViewController alloc] init];
	if( !vc ) return;
	
	vc.achievementDelegate=(id)_delegate;
	
	_state=4;

	// Keep it ios<6 friendly for now...	
	// [BBIosGame::IosGame()->GetUIAppDelegate()->viewController presentViewController:vc animated:YES completion:nil];
	[BBIosGame::IosGame()->GetUIAppDelegate()->viewController presentModalViewController:vc animated:YES];
}
    
void BBGameCenter::ReportAchievement( float percent,String achievement_ID ){
    
	GKAchievement *achievement=[[GKAchievement alloc] initWithIdentifier:achievement_ID.ToNSString()];
	if( !achievement ) return;
	
	achievement.percentComplete=percent;
	
	[achievement reportAchievementWithCompletionHandler:^(NSError *error){} ];
}
    
void BBGameCenter::GameCenterViewControllerDidFinish( UIViewController *vc ){
    
	_state=2;
    	
   	// Keep it ios<6 friendly for now...	
	// [BBIosGame::IosGame()->GetUIAppDelegate()->viewController dismissViewControllerAnimated:YES completion:nil];
	[BBIosGame::IosGame()->GetUIAppDelegate()->viewController dismissModalViewControllerAnimated:YES];
}
