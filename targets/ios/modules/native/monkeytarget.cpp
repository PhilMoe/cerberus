
class BBMonkeyGame : public BBIosGame{
public:
};

@implementation MonkeyView
@end

@implementation MonkeyWindow
@end

@implementation MonkeyViewController
@end

@implementation MonkeyAppDelegate

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions{

	game=new BBMonkeyGame();
    
    try{
    
		bb_std_main( 0,0 );
    	
    }catch(...){
    
		exit( -1 );
    }
    
    if( !game->Delegate() ) exit( 0 );

	//WUD? Can't set this in IB or it breaks 5.1.1?
	//
	//Note: this may trigger drawView, which will invoke initial StartGame, so it needs to be done *after* delegate is set...
	//	
	if( [_window respondsToSelector:@selector(rootViewController)] ){
        _window.rootViewController=viewController;
	}
	
    [_window makeKeyAndVisible];
    
	return YES;
}

@end
