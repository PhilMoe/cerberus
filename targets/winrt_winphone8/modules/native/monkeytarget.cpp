
// ***** monkeygame.h *****

// This class is the bridge to native project, ie: the only class that can see Direct3DBackground class.
//
class BBMonkeyGame : public BBWinrtGame{
public:
	BBMonkeyGame( Direct3DBackground ^d3dBackground );
	
	//BBMonkeyGame
	void UpdateD3dDevice( ID3D11Device1 *device,ID3D11DeviceContext1 *context,ID3D11RenderTargetView *view );
	bool UpdateGameEx();
	void RotateCoords( float &x,float &y );
	
	//BBWinrtGame implementations...
	virtual int GetDeviceWidth(){ return _background->RenderResolution.Width; }
	virtual int GetDeviceHeight(){ return _background->RenderResolution.Height; }
	virtual int GetDeviceRotation(){ return _background->DeviceRotation; }
	
	virtual ID3D11Device1 *GetD3dDevice(){ return _device.Get(); }
	virtual ID3D11DeviceContext1 *GetD3dContext(){ return _context.Get(); }
	virtual ID3D11RenderTargetView *GetRenderTargetView(){ return _view.Get(); }

	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *format );
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz );

	virtual void ValidateUpdateTimer();
	
	virtual void MouseEvent( int event,int data,float x,float y );
	virtual void TouchEvent( int event,int data,float x,float y );

private:

	Direct3DBackground ^_background;
	
	double _updateTime,_updatePeriod;
	
	ComPtr<ID3D11Device1> _device;
	ComPtr<ID3D11DeviceContext1> _context;
	ComPtr<ID3D11RenderTargetView> _view;
	
};

// ***** monkeygame.cpp *****
BBMonkeyGame::BBMonkeyGame( Direct3DBackground ^d3dBackground ):_background( d3dBackground ),_updateTime( 0 ),_updatePeriod( 0 ){
}

void BBMonkeyGame::UpdateD3dDevice( ID3D11Device1 *device,ID3D11DeviceContext1 *context,ID3D11RenderTargetView *view ){
	_device=device;
	_context=context;
	_view=view;
}

void BBMonkeyGame::ValidateUpdateTimer(){
	if( _updateRate && !_suspended ){
		_updatePeriod=1.0/_updateRate;
		_updateTime=GetTime()+_updatePeriod;
	}else{
		_updatePeriod=_updateTime=0;
	}
}

bool BBMonkeyGame::UpdateGameEx(){
	double time=GetTime();

	if( _suspended || !_updateRate || _updateTime>time ) return false;
	
	for( int i=0;i<4;++i ){
	
		_updateTime+=_updatePeriod;
		
		this->UpdateGame();
		
		if( _suspended || !_updateRate || _updateTime>time ) return true;
	}
	
	_updateTime=GetTime()+_updatePeriod;
	
	return true;
}

void BBMonkeyGame::RotateCoords( float &x,float &y ){
	
	float w=GetDeviceWidth();
	float h=GetDeviceHeight();
	float tx=x,ty=y;
	
	switch( GetDeviceRotation() ){
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
