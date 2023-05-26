
//***** cerberusgame.h *****

class BBIosGame : public BBGame{
public:
	BBIosGame();

	static BBIosGame *IosGame(){ return _iosGame; }
	
	virtual int GetDeviceWidth();
	virtual int GetDeviceHeight();
	virtual void SetKeyboardEnabled( bool enabled );
	virtual void SetUpdateRate( int updateRate );
	virtual int Millisecs();
	virtual int SaveState( String state );
	virtual String LoadState();
	virtual void OpenUrl( String url );
	
	virtual NSURL *PathToNSURL( String path );
	virtual String PathToFilePath( String path );
	
	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *format );
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz );
	virtual AVAudioPlayer *OpenAudioPlayer( String path );
	
	virtual BBCerberusAppDelegate *GetUIAppDelegate();
	
	//***** INTERNAL *****
	
	virtual void StartGame();
	virtual void SuspendGame();
	virtual void ResumeGame();
	
	virtual void UpdateTimerFired();
	virtual void TouchesEvent( UIEvent *event );
	
	virtual void ViewAppeared();
	virtual void ViewDisappeared();
	
protected:
	static BBIosGame *_iosGame;
	
	UIApplication *_app;
	BBCerberusAppDelegate *_appDelegate;
	
	bool _displayLinkAvail;
	UIAccelerometer *_accelerometer;
	NSTimer *_updateTimer;
	id _displayLink;
	
	double _nextUpdate;
	double _updatePeriod;

	UITouch *_touches[32];

	uint64_t _startTime;
	mach_timebase_info_data_t _timeInfo;

	double GetTime();
	void ValidateUpdateTimer();
};

//***** iosgame.cpp *****

#include <AudioToolbox/ExtendedAudioFile.h>
#include <AVFoundation/AVAudioSession.h>
#include <AVFoundation/AVAudioPlayer.h>

unsigned char *LoadWAV( FILE *f,int *length,int *channels,int *format,int *hertz );

BBIosGame *BBIosGame::_iosGame;

BBIosGame::BBIosGame():
_displayLinkAvail( false ),
_accelerometer( 0 ),
_updateTimer( 0 ),
_displayLink( 0 ){
	_iosGame=this;
	
	_app=[UIApplication sharedApplication];
	_appDelegate=(BBCerberusAppDelegate*)[_app delegate];
	
	NSString *reqSysVer=@"3.1";
	NSString *currSysVer=[[UIDevice currentDevice] systemVersion];
	if( [currSysVer compare:reqSysVer options:NSNumericSearch]!=NSOrderedAscending ) _displayLinkAvail=true;

	memset( _touches,0,sizeof(_touches) );
	
	_startTime=mach_absolute_time();
	mach_timebase_info( &_timeInfo );
}

double BBIosGame::GetTime(){
	return (double)( (mach_absolute_time()-_startTime) * _timeInfo.numer / _timeInfo.denom ) / 1000000000.0;
}

void BBIosGame::ValidateUpdateTimer(){

	if( _updateTimer ){
		[_updateTimer invalidate];
		_updateTimer=0;
	}else if( _displayLink ){
		[_displayLink invalidate];
		_displayLink=0;
	}
	
	_nextUpdate=0;
	
	if( _suspended ){
		if( _accelerometer ){
			[_accelerometer setUpdateInterval:0.0];
			[_accelerometer setDelegate:0];
			[_app setIdleTimerDisabled:NO];
			_accelerometer=0;
		}
		return;
	}
	
	if( !_accelerometer && CFG_IOS_ACCELEROMETER_ENABLED ){
		_accelerometer=[UIAccelerometer sharedAccelerometer];
		[_accelerometer setDelegate:_appDelegate];
		[_app setIdleTimerDisabled:YES];
	}
	
	if( _accelerometer ) [_accelerometer setUpdateInterval:1.0/(_updateRate ? _updateRate : 60)];
	
	if( _updateRate==0 || (_updateRate==60 && _displayLinkAvail && CFG_IOS_DISPLAY_LINK_ENABLED) ){
	
		_updatePeriod=1.0/60.0;
				
		_displayLink=[NSClassFromString(@"CADisplayLink") displayLinkWithTarget:_appDelegate selector:@selector(updateTimerFired)];
		[_displayLink setFrameInterval:1];
		[_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		
	}else{

		_updatePeriod=1.0/_updateRate;
		
		NSTimeInterval interval=(NSTimeInterval)( _updatePeriod );
		_updateTimer=[NSTimer scheduledTimerWithTimeInterval:interval target:_appDelegate selector:@selector(updateTimerFired) userInfo:nil repeats:TRUE];
		[[NSRunLoop mainRunLoop] addTimer:_updateTimer forMode:NSRunLoopCommonModes];		
	}
}

//***** BBGame *****

int BBIosGame::GetDeviceWidth(){
	return _appDelegate->view->backingWidth;
}

int BBIosGame::GetDeviceHeight(){
	return _appDelegate->view->backingHeight;
}

void BBIosGame::SetKeyboardEnabled( bool enabled ){
	BBGame::SetKeyboardEnabled( enabled );
	
	if( enabled ){
		_appDelegate->textFieldState=1;
		_appDelegate->textField.text=@" ";
		[_appDelegate->textField becomeFirstResponder];
	}else{
		_appDelegate->textFieldState=0;
		[_appDelegate->textField resignFirstResponder];
	}
}

void BBIosGame::SetUpdateRate( int hertz ){
	if( !hertz && !_displayLinkAvail ) hertz=60;
	BBGame::SetUpdateRate( hertz );
	ValidateUpdateTimer();
}

int BBIosGame::Millisecs(){
	return (int)( (mach_absolute_time()-_startTime) * _timeInfo.numer / (_timeInfo.denom * 1000000L) );
}

int BBIosGame::SaveState( String state ){
	NSString *nsstr=state.ToNSString();
	NSUserDefaults *prefs=[NSUserDefaults standardUserDefaults];
	[prefs setObject:nsstr forKey:@".cerberusstate"];
	[[NSUserDefaults standardUserDefaults] synchronize];
	return 0;
}

String BBIosGame::LoadState(){
	NSUserDefaults *prefs=[NSUserDefaults standardUserDefaults];
	NSString *nsstr=[prefs stringForKey:@".cerberusstate"];
	if( nsstr ) return String( nsstr );
	return "";
}

void BBIosGame::OpenUrl( String url ){
	NSURL *nsurl=[NSURL URLWithString:url.ToNSString()];
	if( nsurl ) [[UIApplication sharedApplication] openURL:nsurl];
}

NSURL *BBIosGame::PathToNSURL( String path ){
	String fpath=PathToFilePath( path );
	if( fpath!="" ) return [NSURL fileURLWithPath:fpath.ToNSString()];
	return [NSURL URLWithString:path.ToNSString()];
}

String BBIosGame::PathToFilePath( String path ){
	if( path.StartsWith( "cerberus://data/" ) ){
		path=path.Slice( 16 );
		NSString *nspath=path.ToNSString();
		NSString *ext=[nspath pathExtension];
		NSString *file=[[nspath lastPathComponent] stringByDeletingPathExtension];
		NSString *dir=[@"data/" stringByAppendingString:[nspath stringByDeletingLastPathComponent]];
		NSString *rpath=[[NSBundle mainBundle] pathForResource:file ofType:ext inDirectory:dir];
		return String( rpath );
	}else if( path.StartsWith( "cerberus://internal/" ) ){
		NSString *docs=[@"~/Documents" stringByExpandingTildeInPath];
		return String( docs )+"/"+path.Slice( 20 );
	}
	return "";
}

unsigned char *BBIosGame::LoadImageData( String path,int *width,int *height,int *format ){

	//need this in case we're running on a thread...
	NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];

	UIImage *image=0;
	
	if( path.StartsWith( "cerberus://data/" ) ){

		path=String( "data/" )+path.Slice(16);
		image=[UIImage imageNamed:path.ToNSString()];

	}else{
	
		if( NSURL *url=PathToNSURL( path ) ){
			if( NSData *data=[NSData dataWithContentsOfURL:url] ){
				image=[UIImage imageWithData:data];
			}
		}
	}
	
	if( !image ){
		[pool release];
		return 0;
	}
	
	CGImageRef cgimage=image.CGImage;
	
	int iwidth=CGImageGetWidth( cgimage );
	int iheight=CGImageGetHeight( cgimage );
	
	unsigned char *data=(unsigned char*)calloc( iwidth*iheight,4 );
	
	CGContextRef context=CGBitmapContextCreate( data,iwidth,iheight,8,iwidth*4,CGImageGetColorSpace(cgimage),kCGImageAlphaPremultipliedLast );
	CGContextDrawImage( context,CGRectMake(0,0,iwidth,iheight),cgimage );
	CGContextRelease( context );
	
	*width=iwidth;
	*height=iheight;
	*format=4;
	
	[pool release];
	
	gc_ext_malloced( (*width)*(*height)*(*format) );
	
	return data;
}

unsigned char *BBIosGame::LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){

	NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];

	if( path.ToLower().EndsWith( ".wav" ) ){
		unsigned char *data=0;
		if( FILE *f=OpenFile( path,"rb" ) ){
			data=LoadWAV( f,length,channels,format,hertz );
			fclose( f );
		}
		[pool release];
		
		if( data ) gc_ext_malloced( (*length)*(*channels)*(*format) );

		return data;
	}
	
	NSURL *url=PathToNSURL( path );
	
    ExtAudioFileRef fileRef=0;
    
	if( url && !ExtAudioFileOpenURL( (CFURLRef)url,&fileRef ) ){

	    AudioStreamBasicDescription fileFormat;
		UInt32 propSize=sizeof( fileFormat );
		
		if( !ExtAudioFileGetProperty( fileRef,kExtAudioFileProperty_FileDataFormat,&propSize,&fileFormat ) ){

		    AudioStreamBasicDescription outputFormat;
		
			outputFormat.mSampleRate=fileFormat.mSampleRate;
			outputFormat.mChannelsPerFrame=fileFormat.mChannelsPerFrame;
			outputFormat.mFormatID=kAudioFormatLinearPCM;
			outputFormat.mBytesPerPacket=2*outputFormat.mChannelsPerFrame;
			outputFormat.mFramesPerPacket=1;
			outputFormat.mBytesPerFrame=2*outputFormat.mChannelsPerFrame;
			outputFormat.mBitsPerChannel=16;
			outputFormat.mFormatFlags=kAudioFormatFlagsNativeEndian|kAudioFormatFlagIsPacked|kAudioFormatFlagIsSignedInteger;
			
			if( !ExtAudioFileSetProperty( fileRef,kExtAudioFileProperty_ClientDataFormat,sizeof( outputFormat ),&outputFormat ) ){

				SInt64 fileLen=0;
				UInt32 propSize=sizeof( fileLen );
				
				if( !ExtAudioFileGetProperty( fileRef,kExtAudioFileProperty_FileLengthFrames,&propSize,&fileLen ) ){
				
					UInt32 dataSize=fileLen * outputFormat.mBytesPerFrame;

					//Why dataSize*2? Sample code does it, but it appears unecessary....
					//
					unsigned char *data=(unsigned char*)calloc( dataSize,2 );

					AudioBufferList buf;
					buf.mNumberBuffers=1;
					buf.mBuffers[0].mData=data;
					buf.mBuffers[0].mDataByteSize=dataSize*2;
					buf.mBuffers[0].mNumberChannels=outputFormat.mChannelsPerFrame;
					
					// Read the data into an AudioBufferList
					if( !ExtAudioFileRead( fileRef,(UInt32*)&fileLen,&buf ) ){
				
						*length=fileLen;
						*channels=outputFormat.mChannelsPerFrame;
						*format=2;
						*hertz=outputFormat.mSampleRate;
						
						ExtAudioFileDispose( fileRef );		
						[pool release];	
						
						gc_ext_malloced( (*length)*(*channels)*(*format) );
		
						return data;
					}
					free( data );
				}
			}
		}
		ExtAudioFileDispose( fileRef );
	}
	[pool release];
	return 0;
}

AVAudioPlayer *BBIosGame::OpenAudioPlayer( String path ){

	NSURL *url=PathToNSURL( path );
	if( !url) return 0;
	
	AVAudioPlayer *player=[[AVAudioPlayer alloc] initWithContentsOfURL:url error:0];
	
	return player;
}

BBCerberusAppDelegate *BBIosGame::GetUIAppDelegate(){
	return _appDelegate;
}

//***** INTERNAL *****

void BBIosGame::StartGame(){
	BBGame::StartGame();
	if( !_updateTimer && !_displayLink ) ValidateUpdateTimer();
}

void BBIosGame::SuspendGame(){
	if( !_started ) return;
	BBGame::SuspendGame();
	ValidateUpdateTimer();
	
}

void BBIosGame::ResumeGame(){
	if( !_started ) return;
	BBGame::ResumeGame();
	ValidateUpdateTimer();
}

void BBIosGame::UpdateTimerFired(){
	if( _suspended ) return;
	
	if( !_updateRate ){
		UpdateGame();
		RenderGame();
		return;
	}
	
	if( !_nextUpdate ) _nextUpdate=GetTime();
	
	int i=0;
	for( ;i<4;++i ){
	
		UpdateGame();
		if( !_nextUpdate ) break;
		
		_nextUpdate+=_updatePeriod;
		if( GetTime()<_nextUpdate ) break;

	}
	if( i==4 ) _nextUpdate=0;
	RenderGame();
}

void BBIosGame::TouchesEvent( UIEvent *event ){
	if( [event type]==UIEventTypeTouches ){
	
		UIView *view=_appDelegate->view;

		float scaleFactor=1.0f;
		if( [view respondsToSelector:@selector(contentScaleFactor)] ){
			scaleFactor=[view contentScaleFactor];
		}
		
		for( UITouch *touch in [event touchesForView:view] ){
		
			int pid;
			for( pid=0;pid<32 && _touches[pid]!=touch;++pid ) {}

			int ev=BBGameEvent::None;

			switch( [touch phase] ){
			case UITouchPhaseBegan:
//				puts( "touches began" );
				if( pid!=32 ) break;
				for( pid=0;pid<32 && _touches[pid];++pid ){}
				if( pid==32 ) break;
				_touches[pid]=touch;
				ev=BBGameEvent::TouchDown;
				break;
			case UITouchPhaseEnded:
//				puts( "touches ended" );
				if( pid==32 ) break;
				_touches[pid]=0;
				ev=BBGameEvent::TouchUp;
				break;
			case UITouchPhaseCancelled:
//				puts( "touches cancelled" );
				if( pid==32 ) break;
				_touches[pid]=0;
				ev=BBGameEvent::TouchUp;
				break;
			case UITouchPhaseMoved:
			case UITouchPhaseStationary:
//				puts( "touches moved/stationery" );
				ev=BBGameEvent::TouchMove;
				break;
			}
			
			if( ev==BBGameEvent::None ) continue;

			CGPoint p=[touch locationInView:view];
			p.x*=scaleFactor;
			p.y*=scaleFactor;
			
			TouchEvent( ev,pid,p.x,p.y );
		}
	}
}

void BBIosGame::ViewAppeared(){
//	puts( "ViewAppeared" );
}

void BBIosGame::ViewDisappeared(){
//	puts( "ViewDisappeared" );
	memset( _touches,0,sizeof(_touches) );
}

//***** BBCerberusView implementation *****

@implementation BBCerberusView

+(Class)layerClass{
	return [MGLLayer class];
}

-(id)initWithCoder:(NSCoder*)coder{

	backingWidth=0;
	backingHeight=0;
	defaultFramebuffer=0;
	colorRenderbuffer=0;
	depthRenderbuffer=0;

	if( self=[super initWithCoder:coder] ){
	
		// Enable retina display
		if( CFG_IOS_RETINA_ENABLED ){
			if( [self respondsToSelector:@selector(contentScaleFactor)] ){
				float scaleFactor=[[UIScreen mainScreen] scale];
				[self setContentScaleFactor:scaleFactor];
			}
		}
    
		// Get the layer
		MGLLayer *mglLayer = (MGLLayer *)self.layer;

        mglLayer.opaque = YES;
        // In this application, we want to retain the MGLDrawable contents after a call to present.
        mglLayer.retainedBacking = YES;
        mglLayer.drawableColorFormat = MGLDrawableColorFormatRGBA8888;

        // Set the layer's scale factor as you wish
        //mglLayer.contentScale = [[UIScreen mainScreen] scale];

		
        if( CFG_OPENGL_GLES20_ENABLED ){
            context=[[MGLContext alloc] initWithAPI:kMGLRenderingAPIOpenGLES2];
            if( !context || ![MGLContext setCurrentContext:context] ) exit(-1);
			
		}else{
		}
	}
	return self;
}

-(void)drawView:(id)sender{
	if( BBIosGame *game=BBIosGame::IosGame() ){
		game->StartGame();
		game->RenderGame();
	}
}

-(void)presentRenderbuffer{
	if( CFG_OPENGL_GLES20_ENABLED ){
        MGLLayer *mglLayer = (MGLLayer *)self.layer;
        [MGLContext setCurrentContext:context forLayer:mglLayer];

        [context present:mglLayer];
	}else{
	}
}

-(BOOL)resizeFromLayer:(MGLLayer *)mglLayer{
    // Allocate color buffer backing based on the current layer size
    if( CFG_OPENGL_GLES20_ENABLED ){
        backingWidth = mglLayer.drawableSize.width;
        backingHeight = mglLayer.drawableSize.height;
    }else{
    }
    
    return YES;
}

-(void)layoutSubviews{
	[self resizeFromLayer:(MGLLayer*)self.layer];
	[self drawView:nil];
}

-(void)dealloc{
	if( CFG_OPENGL_GLES20_ENABLED ){
		glDeleteFramebuffers( 1,&defaultFramebuffer );
		glDeleteRenderbuffers( 1,&colorRenderbuffer );
		if( depthRenderbuffer ) glDeleteRenderbuffers( 1,&depthRenderbuffer );
        if( [MGLContext currentContext]==context ) [MGLContext setCurrentContext:nil];
		[context release];
	}else{
	}
	[super dealloc];
}

@end

//***** BBCerberusWindow implementation *****

@implementation BBCerberusWindow

-(void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event{
	BBIosGame::IosGame()->TouchesEvent( event );
}

-(void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event{
	BBIosGame::IosGame()->TouchesEvent( event );
}

-(void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event{
	BBIosGame::IosGame()->TouchesEvent( event );
}

-(void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event{
	BBIosGame::IosGame()->TouchesEvent( event );
}

@end

//***** BBCerberusViewController implementation *****

@implementation BBCerberusViewController

//ios 2
-(void)viewDidAppear:(BOOL)animated{
	[super viewDidAppear:animated];
	BBIosGame::IosGame()->ViewAppeared();
}


-(void)viewDidDisappear:(BOOL)animated{
	[super viewDidDisappear:animated];
	BBIosGame::IosGame()->ViewDisappeared();
}


//ios 4,5
-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation{

	CFArrayRef array=(CFArrayRef)CFBundleGetValueForInfoDictionaryKey( CFBundleGetMainBundle(),CFSTR("UISupportedInterfaceOrientations") );
	if( !array ) return NO;
	
	CFRange range={ 0,CFArrayGetCount( array ) };

	switch( interfaceOrientation ){
	case UIInterfaceOrientationPortrait:return CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationPortrait") );
	case UIInterfaceOrientationPortraitUpsideDown:return CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationPortraitUpsideDown") );
	case UIInterfaceOrientationLandscapeLeft:return CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationLandscapeLeft") );
	case UIInterfaceOrientationLandscapeRight:return CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationLandscapeRight") );
	}
	return NO;
}

//ios 6
-(BOOL)shouldAutorotate{
	return YES;
}

-(NSUInteger)supportedInterfaceOrientations{
    
	CFArrayRef array=(CFArrayRef)CFBundleGetValueForInfoDictionaryKey( CFBundleGetMainBundle(),CFSTR("UISupportedInterfaceOrientations") );
	if( !array ) return 0;
    
	CFRange range={ 0,CFArrayGetCount( array ) };
    
    NSUInteger mask=0;
    
    if( CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationPortrait") ) ) mask|=UIInterfaceOrientationMaskPortrait;
    if( CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationPortraitUpsideDown") ) ) mask|=UIInterfaceOrientationMaskPortraitUpsideDown;
    if( CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationLandscapeLeft") ) ) mask|=UIInterfaceOrientationMaskLandscapeLeft;
    if( CFArrayContainsValue( array,range,CFSTR("UIInterfaceOrientationLandscapeRight") ) ) mask|=UIInterfaceOrientationMaskLandscapeRight;
    
    return mask;
}

@end

//***** BBCerberusAppDelegate implementation *****

@implementation BBCerberusAppDelegate

@synthesize _window;
@synthesize view;
@synthesize viewController;
@synthesize textField;

-(void)applicationWillResignActive:(UIApplication*)application{

	game->SuspendGame();
}

-(void)applicationDidBecomeActive:(UIApplication*)application{

	game->ResumeGame();
}

-(BOOL)textFieldShouldEndEditing:(UITextField*)textField{

	if( !textFieldState ) return YES;

	game->KeyEvent( BBGameEvent::KeyChar,27 );							//generate Escape
	
	return NO;
}

-(BOOL)textFieldShouldReturn:(UITextField*)textField{

	if( textFieldState ) game->KeyEvent( BBGameEvent::KeyChar,13 );	//generate Return
	
	return NO;
}

-(BOOL)textFieldShouldClear:(UITextField*)textField{

	return NO;
}

-(BOOL)textField:(UITextField*)_textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString*)str{

	if( !textFieldState ) return NO;
		
	int n=[str length];
	
	if( n==0 && range.length==1 ){
		game->KeyEvent( BBGameEvent::KeyChar,8 );						//generate Backspace
	}else if( n==1 && range.length==0 ){
		int chr=[str characterAtIndex:0];
		if( chr>=32 ){
			game->KeyEvent( BBGameEvent::KeyChar,chr );
			_textField.text=@"";											//so textfield only contains last char typed.
			return YES;
		}
	}
	return NO;
}

-(void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)accel{

	float accelX,accelY,accelZ;
	
	switch( viewController.interfaceOrientation ){
	case UIDeviceOrientationPortrait:
		accelX=+accel.x;
		accelY=-accel.y;
		break;
	case UIDeviceOrientationPortraitUpsideDown:
		accelX=-accel.x;
		accelY=+accel.y;
		break;
	case UIDeviceOrientationLandscapeLeft:
		accelX=-accel.y;
		accelY=-accel.x;
		break;
	case UIDeviceOrientationLandscapeRight:
		accelX=+accel.y;
		accelY=+accel.x;
		break;
	default:
		return;
	}
	accelZ=-accel.z;
	
	game->MotionEvent( BBGameEvent::MotionAccel,0,accelX,accelY,accelZ );
}

-(void)updateTimerFired{

	game->UpdateTimerFired();
}

-(void)dealloc{
	[_window release];
	[view release];
	[super dealloc];
}

@end
