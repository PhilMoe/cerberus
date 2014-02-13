
//iosgame.h

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenAL/al.h>
#import <OpenAL/alc.h>
#import <AVFoundation/AVAudioPlayer.h>
#import <AudioToolbox/AudioToolbox.h>

#include <TargetConditionals.h>
#include <mach-o/dyld.h>

#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <typeinfo>
#include <sys/stat.h>
#include <dirent.h>
#include <copyfile.h>
#include <signal.h>
#include <pthread.h>

class BBIosGame;

@class MonkeyView;
@class MonkeyWindow;
@class MonkeyViewController;
@class MonkeyAppDelegate;

@interface BBMonkeyView : UIView{
@public
	EAGLContext *context;
	
	GLint backingWidth;
	GLint backingHeight;
	
	GLuint defaultFramebuffer;
	GLuint colorRenderbuffer;
	GLuint depthRenderbuffer;
}

+(Class)layerClass;
-(id)initWithCoder:(NSCoder*)coder;
-(void)drawView:(id)sender;
-(void)presentRenderbuffer;
-(BOOL)resizeFromLayer:(CAEAGLLayer *)layer;
-(void)layoutSubviews;
-(void)dealloc;

@end

@interface BBMonkeyWindow : UIWindow{
@public
}

-(void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event;
-(void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event;
-(void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event;
-(void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event;

@end

@interface BBMonkeyViewController : UIViewController{
@public
}

//ios 2
-(void)viewDidAppear:(BOOL)animated;
-(void)viewDidDisappear:(BOOL)animated;

//ios 4,5
-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation;

//iOS 6
-(BOOL)shouldAutorotate;
-(NSUInteger)supportedInterfaceOrientations;

@end

@interface BBMonkeyAppDelegate : NSObject<UIApplicationDelegate,UIAccelerometerDelegate>{
@public
	BBIosGame *game;
	MonkeyWindow *_window;	//Ugly '_' to fix clash with UIApplicationDelegate window property
	MonkeyView *view;
	MonkeyViewController *viewController;
	UITextField *textField;
	int textFieldState;
}
-(void)applicationWillResignActive:(UIApplication*)application;
-(void)applicationDidBecomeActive:(UIApplication*)application;
-(BOOL)textFieldShouldEndEditing:(UITextField*)textField;
-(BOOL)textField:(UITextField*)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString*)str;
-(void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)accel;
-(void)updateTimerFired;
-(void)dealloc;
	
@property (nonatomic, retain) IBOutlet MonkeyWindow *_window;
@property (nonatomic, retain) IBOutlet MonkeyView *view;
@property (nonatomic, retain) IBOutlet MonkeyViewController *viewController;
@property (nonatomic, retain) IBOutlet UITextField *textField;

@end

//monkeytarget.h

@interface MonkeyView : BBMonkeyView{
}
@end

@interface MonkeyWindow : BBMonkeyWindow{
}
@end

@interface MonkeyViewController : BBMonkeyViewController{
}
@end

@interface MonkeyAppDelegate : BBMonkeyAppDelegate{
}
@end
