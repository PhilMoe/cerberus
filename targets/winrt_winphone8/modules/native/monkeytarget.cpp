
// ***** monkeygame.h *****

// This class is the bridge to native project, ie: the only class that can see Direct3DBackground class.
//
class BBMonkeyGame : public BBWinrtGame{
public:
	BBMonkeyGame( Direct3DBackground ^d3dBackground );

	//BBMonkeyGame
	void UpdateD3dDevice( ID3D11Device1 *device,ID3D11DeviceContext1 *context,ID3D11RenderTargetView *view );
	void UpdateGameEx();
	void RotateCoords( float &x,float &y );
	
	//BBWinrtGame implementations...
	virtual int GetDeviceWidthX(){ return _background->RenderResolution.Width; }
	virtual int GetDeviceHeightX(){ return _background->RenderResolution.Height; }
	virtual int GetDeviceRotationX(){ return _background->DeviceRotation; }
	virtual ID3D11Device1 *GetD3dDevice(){ return _device.Get(); }
	virtual ID3D11DeviceContext1 *GetD3dContext(){ return _context.Get(); }
	virtual ID3D11RenderTargetView *GetRenderTargetView(){ return _view.Get(); }
	virtual void PostToUIThread( std::function<void()> action );
	virtual void RunOnUIThread();

	virtual void ValidateUpdateTimer();
	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *format );
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz );
	
	virtual void MouseEvent( int event,int data,float x,float y );
	virtual void TouchEvent( int event,int data,float x,float y );

private:

	Direct3DBackground ^_background;
	
	std::function<void()> _uiAction;
	
	double _updatePeriod,_nextUpdate;
	
	ComPtr<ID3D11Device1> _device;
	ComPtr<ID3D11DeviceContext1> _context;
	ComPtr<ID3D11RenderTargetView> _view;
	
};

// ***** monkeygame.cpp *****
BBMonkeyGame::BBMonkeyGame( Direct3DBackground ^d3dBackground ):_background( d3dBackground ),_updatePeriod( 0 ),_nextUpdate( 0 ){
}

void BBMonkeyGame::UpdateD3dDevice( ID3D11Device1 *device,ID3D11DeviceContext1 *context,ID3D11RenderTargetView *view ){
	_device=device;
	_context=context;
	_view=view;
}

void BBMonkeyGame::PostToUIThread( std::function<void()> action ){
	_uiAction=action;
	_background->PostToUIThread();
}

void BBMonkeyGame::RunOnUIThread(){
	_uiAction();
	_uiAction=nullptr;
}

void BBMonkeyGame::ValidateUpdateTimer(){
	if( _updateRate ) _updatePeriod=1.0/_updateRate;
	_nextUpdate=0;
}

void BBMonkeyGame::UpdateGameEx(){
	if( _suspended ) return;
	
	if( !_updateRate ){
		UpdateGame();
		return;
	}
	
	if( !_nextUpdate ) _nextUpdate=GetTime();
	
	for( int i=0;i<4;++i ){

		UpdateGame();
		if( !_nextUpdate ) return;
		
		_nextUpdate+=_updatePeriod;
		if( GetTime()<_nextUpdate ) return;
	}
	_nextUpdate=0;
}

void BBMonkeyGame::RotateCoords( float &x,float &y ){
	
	float w=GetDeviceWidthX();
	float h=GetDeviceHeightX();
	float tx=x,ty=y;
	
	switch( GetDeviceRotationX() ){
	case 0:
		break;
	case 1:
		x=ty;
		y=w-tx-1;
		break;
	case 2:
		x=w-tx-1;
		y=h-ty-1;
		break;
	case 3:
		x=h-ty-1;
		y=tx;
		break;
	}
}

unsigned char *BBMonkeyGame::LoadImageData( String path,int *width,int *height,int *format ){

	FILE *f=OpenFile( path,"rb" );
	if( !f ) return 0;
	
	unsigned char *data=stbi_load_from_file( f,width,height,format,0 );

	fclose( f );
	
	if( data ) gc_ext_malloced( (*width)*(*height)*(*format) );

	return data;
}

unsigned char *BBMonkeyGame::LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){

	FILE *f=OpenFile( path,"rb" );
	if( !f ) return 0;
	
	unsigned char *data=0;
	
	if( path.ToLower().EndsWith( ".wav" ) ){
	
		data=LoadWAV( f,length,channels,format,hertz );
		
	}else if( path.ToLower().EndsWith( ".ogg" ) ){
	
//		data=loadOGG( f,length,channels,format,hertz );
	}
	
	fclose( f );
	
	if( data ) gc_ext_malloced( (*length)*(*channels)*(*format) );
	
	return data;
}

void BBMonkeyGame::MouseEvent( int event,int data,float x,float y ){
	RotateCoords( x,y );
	BBWinrtGame::MouseEvent( event,data,x,y );
}

void BBMonkeyGame::TouchEvent( int event,int data,float x,float y ){
	RotateCoords( x,y );
	BBWinrtGame::TouchEvent( event,data,x,y );
}
