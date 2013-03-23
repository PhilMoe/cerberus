
// ***** app.h *****

#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

class BBApp : public Object{
public:
	//Monkey interface - called by monkey code like mojo
	virtual int Width();
	virtual int Height();
	virtual float AccelX();
	virtual float AccelY();
	virtual float AccelZ();
	virtual void SetUpdateRate( int hertz );
	
	//target interface - called by native target code
	virtual void Update();
	virtual void Render();
	virtual void Resize( int width,int height );

	int _width;
	int _height;
	int _hertz;
};

class BBAppDelegate : public Interface{
public:
/*
	virtual void AppStarted()=0;
	virtual void AppSuspended()=0;
	virtual void AppResumed()=0;
*/
	virtual void AppResized( int width,int height )=0;
	virtual void UpdateApp()=0;
	virtual void RenderApp()=0;
/*	
	virtual void KeyDown( int key )=0;
	virtual void KeyUp( int key )=0;
	virtual void CharHit( int char )=0;
	virtual void MouseDown( int button,float x,float y )=0;
	virtual void MouseMoved( int button,float x,float y )=0;
	virtual void MouseUp( int button,float x,float y )=0;
	virtual void TouchDown( int finger,float x,float y )=0;
	virtual void TouchMoved( int finger,float x,float y )=0;
	virtual void TouchUp( int finger,float x,float y )=0;
*/
};

#endif

// ***** app.cpp *****

BBApp::BBApp():
_width(0),_height(0),_hertz(0){
}

int BBApp::Width(){
	return _width;
}

int BBApp::Height(){
	return _height;
}

float BBApp::AccelX(){
	return 0;
}

float BBApp::AccelY(){
	return 0;
}

float BBApp::AccelZ(){
	return 0;
}

void BBApp::SetUpdateRate( int hertz ){
	_hertz=hertz;
}

void BBApp::Resize( int width,int height ){
	_width=width;
	_height=height;
	if( _delegate ) _delegate->AppResized( width,height );
}

void BBApp::Update(){
	if( _delegate ) _delegate->UpdateApp();

}

void BBApp::Render(){
	if( _delegate ) _delegate->RenderApp();
}





