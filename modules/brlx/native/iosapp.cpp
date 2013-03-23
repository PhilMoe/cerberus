
// ***** iosapp.h *****

#ifndef IOSAPP_H_INCLUDED
#define IOSAPP_H_INCLUDED

class IosApp;
class IosAppDelegate;

extern IosApp *TheIosApp;

class IosApp : public App{
public:

	IosApp( UIView *view );
	
	int Millisecs();
	void RequestRender();
	void SetUpdateRate( int hertz );
	void SetAppDelegate( AppDelegate *delegate );
	void SetIosAppDelegate( IosAppDelegate *delegate );
	
	AppDelegate *GetAppDelegate();
	IosAppDelegate *GetIosAppDelegate();

	// INTERNAL - targets should call these!
	void Idle();
	void Start();
	void Resume();
	void Suspend();	
	void Event( UIEvent *event );

	// PRIVATE	
	void Update();

private:

	enum{
		STATE_INIT=0,
		STATE_RUNNING=1,
		STATE_SUSPENDED=2
	};

	UIView *_view;
	int _state;
	int _updateRate;
	id _updateTimer;
	bool _renderReq;
	UITouch *_touches[32];
	AppDelegate *_appDelegate;
	IosAppDelegate *_iosAppDelegate;
};

Class IosAppDelegate : public AppDelegate{
};

#endif

// ***** iosapp.cpp *****

IosApp *TheIosApp;

@interface UpdateTimer{
	int _updateRate;
	NSTimer *_timer;
	id _displayLink;
}

-(id)initWithUpdateRate:(int)updateRate;
-(void)stop;
-(void)timerFired;
-(void)dealloc;

@end

@implementation UpdateTimer

-(id)initWithUpdateRate:(int)updateRate timerTarget:(id)target timerSel:(SEL)sel{
	_updateRate=updateRate;
	
	if( _updateRate==60 ){
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if( [currSysVer compare:reqSysVer options:NSNumericSearch]!=NSOrderedAscending ){
			_timer=0;
			_displayLink=[NSClassFromString(@"CADisplayLink") displayLinkWithTarget:appDelegate selector:@selector(updateTimerFired)];
			[_displayLink setFrameInterval:1];
			[_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
			return self;
		}
	}
	_displayLink=0;
	_timer=[NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)(1.0/updateRate) target:self selector:@selector(timerFired) userInfo:nil repeats:TRUE];
	return self;
}

-(void)stop{
	if( _timer ){
		[_timer invalidate];
		_timer=0;
	}else if( _displayLink ){
		[_displayLink invalidate];
		_displayLink=0;
	}
}

-(void)timerFired{
	TheIosApp->Update();
}

-(void)dealloc{
	[self stop];
}

@end

IosApp::IosApp( UIView *view ):
_view(view),
_state(STATE_INIT),
_updateTimer(0),
_renderReq(false),
_appDelegate(0),
_iosAppDelegate(0){

	memset( _touches,0,sizeof(_touches) );

	if( TheIosApp ) exit(-1);
	TheIosApp=this;
}

int IosApp::Width(){
	return [_view frame].size.width;
}

int IosApp::Height(){
	return [_view frame].size.height;
}

int IosApp::Millisecs(){
}

void IosApp::RequestRender(){
	_renderReq=true;
}

void IosApp::SetUpdateRate( int hertz ){
	if( _updateTimer ) [_updateTimer release];
	if( _updateRate=hertz ){
		_updateTimer=[[UpdateTimer alloc]initWithUpdateRate:hertz];
	}else{
		_updateTimer=0;
	}
}

void IosApp::SetAppDelegate( AppDelegate *delegate ){
	_appDelegate=delegate;
}

void IosApp::SetIosAppDelegate( IosAppDelegate *delegate ){
	_iosAppDelegate=delegate;
}

void IosApp::Idle(){
	if( _renderReq && Width() && Height() ){
		_renderReq=false;
		_appDelegate->RenderApp();
	}
}

void IosApp::Start(){
	switch( _state ){
	case STATE_INIT:
		_state=STATE_RUNNING;
		_appDelegate->StartApp();
		break;
	}
}

void IosApp::Suspend(){
	switch( _state ){
	case STATE_RUNNING:
		_state=STATE_SUSPENDED;
		_appDelegate->SuspendApp();
		break;
	}
}

void IosApp::Resume(){
	switch( _state ){
	case STATE_INIT:
		Start();
		break;
	case STATE_SUSPENDED:
		_state=STATE_RUNNING;
		_appDelegate->ResumeApp();
		break;
	}
}

void IosApp::Update(){
	switch( _state ){
	case STATE_RUNNING:
		_apDelegate->UpdateApp();
		break;
	}
}

void IosApp::Event( UIEvent *event ){

	if( [event type]==UIEventTypeTouches ){
	
		float scaleFactor=1.0f;
		if( [_view respondsToSelector:@selector(contentScaleFactor)] ){
			scaleFactor=[_view contentScaleFactor];
		}
		
		for( int pid=0;pid<32;++pid ){
			if( _touches[pid] && _touches[pid].view!=_view ) _touches[pid]=0;
		}

		for( UITouch *touch in [event touchesForView:_view] ){
			int pid;
			for( pid=0;pid<32 && _touches[pid]!=touch;++pid ) {}
			
			int op=0;
			switch( [touch phase] ){
			case UITouchPhaseBegan:
				if( pid!=32 ){ pid=32;break; }
				for( pid=0;pid<32 && _touches[pid];++pid ){}
				if( pid==32 ) break;
				_touches[pid]=touch;
				op=1;
				break;
			case UITouchPhaseMoved:
			case UITouchPhaseStationary:
				op=2;
				break;
			case UITouchPhaseEnded:
			case UITouchPhaseCancelled:
				if( pid==32 ) break;
				_touches[pid]=0;
				op=3;
				break;
			}
			if( op ){
				CGPoint p=[touch locationInView:_view];
				p.x*=scaleFactor;
				p.y*=scaleFactor;
				switch(op){
				case 1:_appDelegate->TouchDown( pid,p.x,p.y );break;
				case 2:_appDelegate->TouchMoved( pid,p.x,p.y );break;
				case 3:_appDelegate->TouchUp( pid );break;
				}
			}
		}
	}
}
