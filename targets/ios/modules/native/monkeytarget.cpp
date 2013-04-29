
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

	
	//WUD? Can't set this in IB or it breaks 5.1.1?
	//
	if( [_window respondsToSelector:@selector(rootViewController)] ){
        _window.rootViewController=viewController;
	}
	
    [_window makeKeyAndVisible];
    
	game=new BBMonkeyGame();
    
    try{
    
		bb_std_main( 0,0 );
    	
    }catch(...){
    
		exit( -1 );
    }
    
    if( !game->Delegate() ) exit( 0 );
    
	game->StartGame();

	return YES;
}

@end
