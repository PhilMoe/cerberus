
//***** win8game.h *****

class BBWin8Game : public BBGame{
public:
	BBWin8Game();

	static BBWin8Game *Win8Game(){ return _win8Game; }
	
	static void Main( int argc,const char *argv[] );
	
	virtual void SetKeyboardEnabled( bool enabled );
	virtual void SetUpdateRate( int updateRate );
	virtual int Millisecs();
	virtual int SaveState( String state );
	virtual String LoadState();
	virtual bool PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons );
	virtual void OpenUrl( String url );
	
	virtual void RenderGame();
	
	virtual String PathToFilePath( String path );
	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *format );
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz );

	virtual void RotateCoords( float &x,float &y );
	virtual void MouseEvent( int event,int data,float x,float y );
	virtual void TouchEvent( int event,int data,float x,float y );
	
	virtual void ValidateOrientation();
	
	virtual int  GetDeviceWidth(){ return _deviceWidth; }
	virtual int  GetDeviceHeight(){ return _deviceHeight; }
	virtual int  GetDeviceRotation(){ return _deviceRotation; }
	
	virtual ID3D11Device1 *GetD3dDevice(){ return _d3dDevice; }
	virtual ID3D11DeviceContext1 *GetD3dContext(){ return _d3dContext; }
	virtual IDXGISwapChain1 *GetSwapChain(){ return _swapChain; }
	virtual ID3D11RenderTargetView *GetRenderTargetView(){ return _renderTargetView; }
	virtual ID3D11DepthStencilView *GetDepthStencilView(){ return _depthStencilView; }
	
private:
	static BBWin8Game *_win8Game;
	
	unsigned int _pointerIds[32];
	
	double _nextUpdate;
	double _updatePeriod;
	
	int _deviceWidth;
	int _deviceHeight;
	int _deviceRotation;
	int _inputRotation;
	
	ID3D11Device1 *_d3dDevice;
	ID3D11DeviceContext1 *_d3dContext;
	IDXGISwapChain1 *_swapChain;
	ID3D11RenderTargetView *_renderTargetView;
	ID3D11DepthStencilView *_depthStencilView;

	D3D_FEATURE_LEVEL _featureLevel;

#if WINDOWS_8	
	IWICImagingFactory *_wicFactory;
#endif
	
	void Run();
	double GetTime();
	void Sleep( double time );
	void PollEvents();
	void WaitEvents();

	void CreateD3dDevice();
};

//***** win8game.cpp *****

#define ZEROMEM(X) memset( &X,0,sizeof(X) );

static void DXASS( HRESULT hr ){
	if( FAILED( hr ) ){
		// Set a breakpoint on this line to catch Win32 API errors.
		throw Platform::Exception::CreateException( hr );
	}
}

static float DipsToPixels( float dips ){
	static const float dipsPerInch=96.0f;
	return floor( dips*DisplayProperties::LogicalDpi/dipsPerInch+0.5f ); // Round to nearest integer.
}

static int DeviceOrientation(){
	switch( DisplayProperties::CurrentOrientation ){
	case DisplayOrientations::Portrait:return 0;
	case DisplayOrientations::Landscape:return 1;
	case DisplayOrientations::PortraitFlipped:return 2;
	case DisplayOrientations::LandscapeFlipped:return 3;
	}
	return 1;
}

static int DeviceRotation(){
	int rot=DeviceOrientation();
#if WINDOWS_8
	return (rot-1)&3;
#else
	return rot;
#endif
}

static DXGI_MODE_ROTATION SwapChainRotation(){
	switch( DeviceRotation() ){
	case 0:return DXGI_MODE_ROTATION_IDENTITY;
	case 1:return DXGI_MODE_ROTATION_ROTATE90;
	case 2:return DXGI_MODE_ROTATION_ROTATE180;
	case 3:return DXGI_MODE_ROTATION_ROTATE270;
	}
	return DXGI_MODE_ROTATION_UNSPECIFIED;
}

BBWin8Game *BBWin8Game::_win8Game;

BBWin8Game::BBWin8Game():
_nextUpdate( 0 ),
_updatePeriod( 0 ),
_d3dDevice( 0 ),
_d3dContext( 0 ),
_swapChain( 0 ),
_renderTargetView( 0 ),
_depthStencilView( 0 )
#if WINDOWS_8
,_wicFactory( 0 )
#endif
{
	_win8Game=this;
	
	CoreWindow ^window=CoreWindow::GetForCurrentThread();

	_deviceWidth=DipsToPixels( window->Bounds.Width );
	_deviceHeight=DipsToPixels( window->Bounds.Height );

#if WINDOWS_8
	switch( DisplayProperties::CurrentOrientation ){
	case DisplayOrientations::Portrait:
	case DisplayOrientations::PortraitFlipped:
		std::swap( _deviceWidth,_deviceHeight );
		break;
	}
#endif
	
	CreateD3dDevice();
}

void BBWin8Game::Main( int argc,const char *argv[] ){

	new BBWin8Game();

	try{
	
		bb_std_main( 0,0 );//argc,argv );
		
	}catch(...){
	
		return;
	}
	
	if( !_win8Game->Delegate() ) return;
	
	_win8Game->Run();
}

void BBWin8Game::SetKeyboardEnabled( bool enabled ){
	BBGame::SetKeyboardEnabled( enabled );

#if WINDOWS_PHONE_8
	CoreWindow::GetForCurrentThread()->IsKeyboardInputEnabled=enabled;
#endif
}

void BBWin8Game::SetUpdateRate( int hertz ){
	BBGame::SetUpdateRate( hertz );
	
	if( _updateRate ){
		_updatePeriod=1.0/_updateRate;
		_nextUpdate=GetTime()+_updatePeriod;
	}
}

int BBWin8Game::Millisecs(){
	return int( GetTime()*1000.0 );
}

int BBWin8Game::SaveState( String state ){
	if( FILE *f=OpenFile( "monkey://internal/.monkeystate","wb" ) ){
		bool ok=state.Save( f );
		fclose( f );
		return ok ? 0 : -2;
	}
	return -1;
}

String BBWin8Game::LoadState(){
	if( FILE *f=OpenFile( "monkey://internal/.monkeystate","rb" ) ){
		String str=String::Load( f );
		fclose( f );
		return str;
	}
	return "";
}

bool BBWin8Game::PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons ){
	return false;
}

void BBWin8Game::OpenUrl( String url ){
	auto str=ref new Platform::String( url.ToCString<char16>(),url.Length() );
	auto uri=ref new Windows::Foundation::Uri( str );
	Windows::System::Launcher::LaunchUriAsync( uri );
}

void BBWin8Game::RenderGame(){
	if( !_started ) return;
	BBGame::RenderGame();
	DXASS( BBWin8Game::Win8Game()->GetSwapChain()->Present( 1,0 ) );
	_d3dContext->DiscardView( BBWin8Game::Win8Game()->GetRenderTargetView() );
}

String BBWin8Game::PathToFilePath( String path ){
	if( !path.StartsWith( "monkey:" ) ){
		return path;
	}else if( path.StartsWith( "monkey://data/" ) ){
		auto folder=Windows::ApplicationModel::Package::Current->InstalledLocation;
		return String( folder->Path )+"/Assets/monkey/"+path.Slice( 14 );
	}else if( path.StartsWith( "monkey://internal/" ) ){
		auto folder=Windows::Storage::ApplicationData::Current->LocalFolder;
		return String( folder->Path )+"/"+path.Slice( 18 );
	}
	return "";
}

#if WINDOWS_8

//***** Windows 8 Version *****

unsigned char *BBWin8Game::LoadImageData( String path,int *pwidth,int *pheight,int *pformat ){

	if( !_wicFactory ){
		DXASS( CoCreateInstance( CLSID_WICImagingFactory,0,CLSCTX_INPROC_SERVER,__uuidof(IWICImagingFactory),(LPVOID*)&_wicFactory ) );
	}
	
	path=PathToFilePath( path );

	IWICBitmapDecoder *decoder;
	if( !SUCCEEDED( _wicFactory->CreateDecoderFromFilename( path.ToCString<wchar_t>(),NULL,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder ) ) ){
		return 0;
	}
	
	unsigned char *data=0;

	IWICBitmapFrameDecode *bitmapFrame;
	DXASS( decoder->GetFrame( 0,&bitmapFrame ) );
	
	UINT width,height;
	WICPixelFormatGUID pixelFormat;
	DXASS( bitmapFrame->GetSize( &width,&height ) );
	DXASS( bitmapFrame->GetPixelFormat( &pixelFormat ) );
			
	if( pixelFormat==GUID_WICPixelFormat24bppBGR ){
		unsigned char *t=(unsigned char*)malloc( width*3*height );
		DXASS( bitmapFrame->CopyPixels( 0,width*3,width*3*height,t ) );
		data=(unsigned char*)malloc( width*4*height );
		unsigned char *s=t,*d=data;
		int n=width*height;
		while( n-- ){
			*d++=s[2];
			*d++=s[1];
			*d++=s[0];
			*d++=0xff;
			s+=3;
		}
		free( t );
	}else if( pixelFormat==GUID_WICPixelFormat32bppBGRA ){
		unsigned char *t=(unsigned char*)malloc( width*4*height );
		DXASS( bitmapFrame->CopyPixels( 0,width*4,width*4*height,t ) );
		data=t;
		int n=width*height;
		while( n-- ){	//premultiply alpha
			unsigned char r=t[0];
			t[0]=t[2]*t[3]/255;
			t[1]=t[1]*t[3]/255;
			t[2]=r*t[3]/255;
			t+=4;
		}
	}
	
	if( data ){
		*pwidth=width;
		*pheight=height;
		*pformat=4;
	}
	
	bitmapFrame->Release();
	decoder->Release();
	
	gc_force_sweep=true;

	return data;
}

//***** Windows 8 Version *****

unsigned char *BBWin8Game::LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){

	String url=PathToFilePath( path );
	
	DXASS( MFStartup( MF_VERSION ) );
	
	IMFAttributes *attrs;
	DXASS( MFCreateAttributes( &attrs,1 ) );
	DXASS( attrs->SetUINT32( MF_LOW_LATENCY,TRUE ) );
	
	IMFSourceReader *reader;
	if( FAILED( MFCreateSourceReaderFromURL( url.ToCString<wchar_t>(),attrs,&reader ) ) ){
		attrs->Release();
		return 0;
	}

	attrs->Release();

	IMFMediaType *mediaType;
	DXASS( MFCreateMediaType( &mediaType ) );
	DXASS( mediaType->SetGUID( MF_MT_MAJOR_TYPE,MFMediaType_Audio ) );
	DXASS( mediaType->SetGUID( MF_MT_SUBTYPE,MFAudioFormat_PCM ) );

	DXASS( reader->SetCurrentMediaType( MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,mediaType ) );
    
	mediaType->Release();

	IMFMediaType *outputMediaType;
	DXASS( reader->GetCurrentMediaType( MF_SOURCE_READER_FIRST_AUDIO_STREAM,&outputMediaType ) );
	
	WAVEFORMATEX *wformat;
	uint32 formatByteCount=0;
	DXASS( MFCreateWaveFormatExFromMFMediaType( outputMediaType,&wformat,&formatByteCount ) );

	*channels=wformat->nChannels;
	*format=wformat->wBitsPerSample/8;
	*hertz=wformat->nSamplesPerSec;

	CoTaskMemFree( wformat );
    
	outputMediaType->Release();
/*    
	PROPVARIANT var;
	DXASS( reader->GetPresentationAttribute( MF_SOURCE_READER_MEDIASOURCE,MF_PD_DURATION,&var ) );
	LONGLONG duration=var.uhVal.QuadPart;
	float64 durationInSeconds=(duration / (float64)(10000 * 1000));
	m_maxStreamLengthInBytes=(uint32)( durationInSeconds * m_waveFormat.nAvgBytesPerSec );
*/
	std::vector<unsigned char*> bufs;
	std::vector<uint32> lens;
	uint32 len=0;
    
	for( ;; ){
		uint32 flags=0;
		IMFSample *sample;
		DXASS( reader->ReadSample( MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,0,reinterpret_cast<DWORD*>(&flags),0,&sample ) );
		
		if( flags & MF_SOURCE_READERF_ENDOFSTREAM ){
			break;
		}
		if( sample==0 ){ 
			abort();
		}
		
		IMFMediaBuffer *mediaBuffer;
		DXASS( sample->ConvertToContiguousBuffer( &mediaBuffer ) );

		uint8 *audioData=0;
		uint32 sampleBufferLength=0;
		DXASS( mediaBuffer->Lock( &audioData,0,reinterpret_cast<DWORD*>( &sampleBufferLength ) ) );
		
		unsigned char *buf=(unsigned char*)malloc( sampleBufferLength );
		memcpy( buf,audioData,sampleBufferLength );
		
		bufs.push_back( buf );
		lens.push_back( sampleBufferLength );
		len+=sampleBufferLength;
		
		DXASS( mediaBuffer->Unlock() );
		mediaBuffer->Release();
		
		sample->Release();
	}
	
	reader->Release();
	
	*length=len/(*channels * *format);

	unsigned char *data=(unsigned char*)malloc( len );
	unsigned char *p=data;
	
	for( int i=0;i<bufs.size();++i ){
		memcpy( p,bufs[i],lens[i] );
		free( bufs[i] );
		p+=lens[i];
	}
	
	gc_force_sweep=true;
	
	return data;
}	

#else

//***** Window Phone 8 Version *****

unsigned char *BBWin8Game::LoadImageData( String path,int *width,int *height,int *format ){

	FILE *f=OpenFile( path,"rb" );
	if( !f ) return 0;
	
	unsigned char *data=stbi_load_from_file( f,width,height,format,0 );
	fclose( f );
	
	
	gc_force_sweep=true;

	return data;
}

//***** Window Phone 8 Version *****

unsigned char *BBWin8Game::LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){

	FILE *f=OpenFile( path,"rb" );
	if( !f ) return 0;
	
	unsigned char *data=0;
	
	if( path.ToLower().EndsWith( ".wav" ) ){
	
		data=LoadWAV( f,length,channels,format,hertz );
		
	}else if( path.ToLower().EndsWith( ".ogg" ) ){
	
//		data=loadOGG( f,length,channels,format,hertz );
	}
	
	fclose( f );
	
	gc_force_sweep=true;
	
	return data;
}

#endif

void BBWin8Game::ValidateOrientation(){

	int devrot=DeviceRotation();
	if( CFG_WIN8_SCREEN_ORIENTATION & (1<<DeviceOrientation()) ) _deviceRotation=devrot;

#if WINDOWS_8

	_inputRotation=(4-devrot+_deviceRotation)&3;
	if( _swapChain ) _swapChain->SetRotation( SwapChainRotation() );
	Windows::UI::Core::CoreWindowResizeManager::GetForCurrentView()->NotifyLayoutCompleted();

#elif WINDOWS_PHONE_8

	_inputRotation=_deviceRotation;

#endif

}

void BBWin8Game::RotateCoords( float &x,float &y ){

	CoreWindow ^window=CoreWindow::GetForCurrentThread();
	
	int width=DipsToPixels( window->Bounds.Width );
	int height=DipsToPixels( window->Bounds.Height );

	float t;
	switch( _inputRotation ){
	case 1:
		t=x;x=y;y=width-t-1;
		break;
	case 2:
		x=width-x-1;y=height-y-1;
		break;
	case 3:
		t=x;x=height-y-1;y=t;
		break;
	}
}

void BBWin8Game::MouseEvent( int event,int data,float x,float y ){
	RotateCoords( x,y );
	BBGame::MouseEvent( event,data,x,y );
}

void BBWin8Game::TouchEvent( int event,int data,float x,float y ){
	RotateCoords( x,y );
	BBGame::TouchEvent( event,data,x,y );
}

void BBWin8Game::Run(){

	DisplayOrientations prefs=DisplayOrientations::None;
	if( CFG_WIN8_SCREEN_ORIENTATION & 1 ) prefs=prefs|DisplayOrientations::Portrait;
	if( CFG_WIN8_SCREEN_ORIENTATION & 2 ) prefs=prefs|DisplayOrientations::Landscape;
	if( CFG_WIN8_SCREEN_ORIENTATION & 4 ) prefs=prefs|DisplayOrientations::PortraitFlipped;
	if( CFG_WIN8_SCREEN_ORIENTATION & 8 ) prefs=prefs|DisplayOrientations::LandscapeFlipped;
	if( prefs==DisplayOrientations::None ) prefs=DisplayProperties::CurrentOrientation;
	
	Windows::Graphics::Display::DisplayProperties::AutoRotationPreferences=prefs;

	int orientation;
	for( orientation=0;orientation<4 && !(CFG_WIN8_SCREEN_ORIENTATION & (1<<orientation));++orientation ) {}
	if( orientation==4 ) orientation=DeviceOrientation();

#if WINDOWS_8
	_deviceRotation=(orientation-1)&3;
#elif WINDOWS_PHONE_8
	_deviceRotation=orientation;
#endif	

	ValidateOrientation();
	
	StartGame();
	
	for(;;){
	
		if( _updateRate==60 ){
			PollEvents();
			UpdateGame();
			RenderGame();
			continue;
		}
	
		if( !_updateRate || _suspended ){
			RenderGame();
			WaitEvents();
			continue;
		}
		
		double time=GetTime();
		if( time<_nextUpdate ){
			Sleep( _nextUpdate-time );
			continue;
		}
		
		PollEvents();
				
		int updates=0;
		for(;;){
			_nextUpdate+=_updatePeriod;
			
			UpdateGame();
			if( !_updateRate ) break;
			
			if( _nextUpdate>GetTime() ){
				break;
			}
			
			if( ++updates==8 ) break;
		}
		RenderGame();
		if( updates==8 ) _nextUpdate=GetTime();
	}
}

double BBWin8Game::GetTime(){
	static int f;
	static LARGE_INTEGER pcf,pc0;
	if( !f ){
		if( QueryPerformanceFrequency( &pcf ) && QueryPerformanceCounter( &pc0 ) ){
			f=1;
		}else{
			f=-1;
		}
	}
	if( f>0 ){
		LARGE_INTEGER pc;
		if( QueryPerformanceCounter( &pc ) ) return (double)(pc.QuadPart-pc0.QuadPart) / (double)pcf.QuadPart;
		f=-1;
	}
	abort();
	return -1;
}

void BBWin8Game::Sleep( double time ){
	if( WaitForSingleObjectEx( GetCurrentThread(),int( time*1000.0 ),FALSE )==WAIT_OBJECT_0 ){
		//success!
	}
}

void BBWin8Game::PollEvents(){
	CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( CoreProcessEventsOption::ProcessAllIfPresent );
	ValidateOrientation();
}

void BBWin8Game::WaitEvents(){
	CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( CoreProcessEventsOption::ProcessOneAndAllPending );
	ValidateOrientation();
}

void BBWin8Game::CreateD3dDevice(){

	CoreWindow ^window=CoreWindow::GetForCurrentThread();

	int width=_deviceWidth;
	int height=_deviceHeight;

	UINT creationFlags=D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	
#ifdef _DEBUG
	creationFlags|=D3D11_CREATE_DEVICE_DEBUG;
#endif

#if WINDOWS_8	
	D3D_FEATURE_LEVEL featureLevels[]={
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
#elif WINDOWS_PHONE_8
	D3D_FEATURE_LEVEL featureLevels[]={
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3
	};
#endif
	
	ID3D11Device *device;
	ID3D11DeviceContext *context;

	DXASS( D3D11CreateDevice( 
		0,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		creationFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&device,
		&_featureLevel,
		&context ) );
		
	DXASS( device->QueryInterface( __uuidof( ID3D11Device1 ),(void**)&_d3dDevice ) );
	DXASS( context->QueryInterface( __uuidof( ID3D11DeviceContext1 ),(void**)&_d3dContext ) );
	
	device->Release();
	context->Release();
	
	//create swap chain
	
	if( _swapChain ){

		DXASS( _swapChain->ResizeBuffers( 2,width,height,DXGI_FORMAT_B8G8R8A8_UNORM,0 ) );

	}else{

#if WINDOWS_8
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc={0};
		swapChainDesc.Width=width;
		swapChainDesc.Height=height;
		swapChainDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo=false;
		swapChainDesc.SampleDesc.Count=1;
		swapChainDesc.SampleDesc.Quality=0;
		swapChainDesc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount=2;
		swapChainDesc.Scaling=DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.Flags=0;
#elif WINDOWS_PHONE_8
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc={0};
		swapChainDesc.Width=width;
		swapChainDesc.Height=height;
		swapChainDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo=false;
		swapChainDesc.SampleDesc.Count=1;
		swapChainDesc.SampleDesc.Quality=0;
		swapChainDesc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount=1;
		swapChainDesc.Scaling=DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags=0;
#endif
		IDXGIDevice1 *dxgiDevice;
		DXASS( _d3dDevice->QueryInterface( __uuidof( IDXGIDevice1 ),(void**)&dxgiDevice ) );
		
		IDXGIAdapter *dxgiAdapter;
		DXASS( dxgiDevice->GetAdapter( &dxgiAdapter ) );

		IDXGIFactory2 *dxgiFactory;
		DXASS( dxgiAdapter->GetParent( __uuidof( IDXGIFactory2 ),(void**)&dxgiFactory ) );
	
		DXASS( dxgiFactory->CreateSwapChainForCoreWindow( _d3dDevice,(IUnknown*)window,&swapChainDesc,0,&_swapChain ) );

		DXASS( dxgiDevice->SetMaximumFrameLatency( 1 ) );
		
		dxgiFactory->Release();
		dxgiAdapter->Release();
		dxgiDevice->Release();
	}
	
	// Create a render target view of the swap chain back buffer.
	//
	ID3D11Texture2D *backBuffer;
	DXASS( _swapChain->GetBuffer( 0,__uuidof( ID3D11Texture2D ),(void**)&backBuffer ) );
	DXASS( _d3dDevice->CreateRenderTargetView( backBuffer,0,&_renderTargetView ) );
	backBuffer->Release();

/*
	// Create a depth stencil view
	//
	D3D11_TEXTURE2D_DESC dsdesc;
	ZEROMEM( dsdesc );
	dsdesc.Width=width;
	dsdesc.Height=height;
	dsdesc.MipLevels=1;
	dsdesc.ArraySize=1;
	dsdesc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsdesc.SampleDesc.Count=1;
	dsdesc.SampleDesc.Quality=0;
	dsdesc.Usage=D3D11_USAGE_DEFAULT;
	dsdesc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
	dsdesc.CpuAccessFlags=0;
	dsdesc.MiscFlags=0;
	ID3D11Texture2D *depthStencil;
	DXASS( _d3dDevice->CreateTexture2D( &dsdesc,0,&depthStencil ) );
	DXASS( _d3dDevice->CreateDepthStencilView( depthStencil,0,&_depthStencilView ) );
	depthStencil->Release();
*/

	D3D11_VIEWPORT viewport={ 0,0,width,height,0,1 };
	_d3dContext->RSSetViewports( 1,&viewport );
}

//***** implements WinRT Win8Game class *****

Win8Game::Win8Game():
_windowClosed( false ),
_windowVisible( true ){
	memset( _pointerIds,0,sizeof( _pointerIds ) );
}

void Win8Game::Initialize( CoreApplicationView ^applicationView ){

	CoreApplication::Suspending+=ref new EventHandler<SuspendingEventArgs^>( this,&Win8Game::OnSuspending );
	CoreApplication::Resuming+=ref new EventHandler<Platform::Object^>( this,&Win8Game::OnResuming );

	applicationView->Activated+=ref new TypedEventHandler<CoreApplicationView^,IActivatedEventArgs^>( this,&Win8Game::OnActivated );
}

void Win8Game::SetWindow( CoreWindow ^window ){

	window->SizeChanged+=ref new TypedEventHandler<CoreWindow^,WindowSizeChangedEventArgs^>( this,&Win8Game::OnWindowSizeChanged );
	window->VisibilityChanged+=ref new TypedEventHandler<CoreWindow^,VisibilityChangedEventArgs^>( this,&Win8Game::OnVisibilityChanged );
	window->Closed+=ref new TypedEventHandler<CoreWindow^,CoreWindowEventArgs^>( this,&Win8Game::OnWindowClosed );
	
#if WINDOWS_8	
	window->PointerCursor=ref new CoreCursor( CoreCursorType::Arrow,0 );
#endif

	window->InputEnabled+=ref new TypedEventHandler<CoreWindow^,InputEnabledEventArgs^>( this,&Win8Game::OnInputEnabled );

	window->KeyDown+=ref new TypedEventHandler<CoreWindow^,KeyEventArgs^>( this,&Win8Game::OnKeyDown );
	window->KeyUp+=ref new TypedEventHandler<CoreWindow^,KeyEventArgs^>( this,&Win8Game::OnKeyUp );
	window->CharacterReceived+=ref new TypedEventHandler<CoreWindow^,CharacterReceivedEventArgs^>( this,&Win8Game::OnCharacterReceived );

	window->PointerPressed+=ref new TypedEventHandler<CoreWindow^,PointerEventArgs^>( this,&Win8Game::OnPointerPressed );
	window->PointerReleased+=ref new TypedEventHandler<CoreWindow^,PointerEventArgs^>( this,&Win8Game::OnPointerReleased );
	window->PointerMoved+=ref new TypedEventHandler<CoreWindow^,PointerEventArgs^>( this,&Win8Game::OnPointerMoved );
	
#if WINDOWS_PHONE_8
	auto inputPane=Windows::UI::ViewManagement::InputPane::GetForCurrentView();
	inputPane->Showing+=ref new TypedEventHandler<Windows::UI::ViewManagement::InputPane^,Windows::UI::ViewManagement::InputPaneVisibilityEventArgs^>( this,&Win8Game::OnInputPaneShowing );
	inputPane->Hiding+=ref new TypedEventHandler<Windows::UI::ViewManagement::InputPane^,Windows::UI::ViewManagement::InputPaneVisibilityEventArgs^>( this,&Win8Game::OnInputPaneHiding );
	Windows::Phone::UI::Input::HardwareButtons::BackPressed+=ref new EventHandler<Windows::Phone::UI::Input::BackPressedEventArgs^>( this,&Win8Game::OnBackButtonPressed );
#endif
}

void Win8Game::Load( Platform::String ^entryPoint ){
}

void Win8Game::Run(){

	BBWin8Game::Main( 0,0 );
}

void Win8Game::Uninitialize(){
}

void Win8Game::OnWindowSizeChanged( CoreWindow ^sender,WindowSizeChangedEventArgs ^args ){
//	Print( "Window Size Changed" );

	BBWin8Game::Win8Game()->ValidateOrientation();
}

void Win8Game::OnVisibilityChanged( CoreWindow ^sender,VisibilityChangedEventArgs ^args ){
//	Print( "Visibility Changed" );

	_windowVisible=args->Visible;
	
	if( _windowVisible ){
		BBWin8Game::Win8Game()->ResumeGame();
	}else{
		BBWin8Game::Win8Game()->SuspendGame();
	}
}

void Win8Game::OnInputEnabled( CoreWindow ^window,InputEnabledEventArgs ^args ){
//	Print( "Input Enabled" );
}

void Win8Game::OnWindowClosed( CoreWindow ^sender,CoreWindowEventArgs ^args ){
//	Print( "Window Closed" );
	_windowClosed=true;
}

void Win8Game::OnKeyDown( CoreWindow ^sender,KeyEventArgs ^args ){
	int data=(int)args->VirtualKey;
	BBWin8Game::Win8Game()->KeyEvent( BBGameEvent::KeyDown,data );
#if WINDOWS_PHONE_8
	if( data==8 ){
		BBWin8Game::Win8Game()->KeyEvent( BBGameEvent::KeyChar,data );
	}
#endif	
}

void Win8Game::OnKeyUp( CoreWindow ^sender,KeyEventArgs ^args ){
	int data=(int)args->VirtualKey;
	BBWin8Game::Win8Game()->KeyEvent( BBGameEvent::KeyUp,data );
}

void Win8Game::OnCharacterReceived( CoreWindow ^sender,CharacterReceivedEventArgs ^args ){
	int data=(int)args->KeyCode;
	BBWin8Game::Win8Game()->KeyEvent( BBGameEvent::KeyChar,data );
}

void Win8Game::OnPointerPressed( CoreWindow ^sender,PointerEventArgs ^args ){

	auto p=args->CurrentPoint;
	
#if WINDOWS_8
	auto t=p->PointerDevice->PointerDeviceType;
#elif WINDOWS_PHONE_8
	auto t=Windows::Devices::Input::PointerDeviceType::Touch;
#endif
	
	switch( t ){
	case Windows::Devices::Input::PointerDeviceType::Touch:
		{
			int id=0;
			while( id<32 && _pointerIds[id]!=p->PointerId ) ++id;
			if( id<32 ) return;		//Error! Pointer ID already in use!
			id=0;
			while( id<32 && _pointerIds[id] ) ++id;
			if( id==32 ) return;	//Error! Too many fingers!
			_pointerIds[id]=p->PointerId;
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			BBWin8Game::Win8Game()->TouchEvent( BBGameEvent::TouchDown,id,x,y );
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		{
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			switch( p->Properties->PointerUpdateKind ){
			case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseDown,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseDown,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseDown,2,x,y );
				break;
			}
		}
		break;
	}
}

void Win8Game::OnPointerReleased( CoreWindow ^sender,PointerEventArgs ^args ){

	auto p=args->CurrentPoint;
	
#if WINDOWS_8
	auto t=p->PointerDevice->PointerDeviceType;
#elif WINDOWS_PHONE_8
	auto t=Windows::Devices::Input::PointerDeviceType::Touch;
#endif
	
	switch( t ){
	case Windows::Devices::Input::PointerDeviceType::Touch:
		{
			int id=0;
			while( id<32 && _pointerIds[id]!=p->PointerId ) ++id;
			if( id==32 ) return; 	//Pointer ID not found!
			_pointerIds[id]=0;
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			BBWin8Game::Win8Game()->TouchEvent( BBGameEvent::TouchUp,id,x,y );
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		{
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			switch( p->Properties->PointerUpdateKind ){
			case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseUp,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseUp,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseUp,2,x,y );
				break;
			}
		}
		break;
	}
}

void Win8Game::OnPointerMoved( CoreWindow ^sender,PointerEventArgs ^args ){

	auto p=args->CurrentPoint;
	
#if WINDOWS_8
	auto t=p->PointerDevice->PointerDeviceType;
#elif WINDOWS_PHONE_8
	auto t=Windows::Devices::Input::PointerDeviceType::Touch;
#endif
	
	switch( t ){
	case Windows::Devices::Input::PointerDeviceType::Touch:
		{
			int id=0;
			while( id<32 && _pointerIds[id]!=p->PointerId ) ++id;
			if( id==32 ) return;	 //Pointer ID not found!
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			BBWin8Game::Win8Game()->TouchEvent( BBGameEvent::TouchMove,id,x,y );
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		{
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			switch( p->Properties->PointerUpdateKind ){
			case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseDown,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseDown,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseDown,2,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseUp,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseUp,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseUp,2,x,y );
				break;
			default:
				BBWin8Game::Win8Game()->MouseEvent( BBGameEvent::MouseMove,-1,x,y );
			}
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Pen:{
		}
		break;
	}
}

void Win8Game::OnActivated( CoreApplicationView ^applicationView,IActivatedEventArgs ^args ){
//	Print( "Activated" );
	CoreWindow::GetForCurrentThread()->Activate();
}

//WP8 only?
void Win8Game::OnSuspending( Platform::Object ^sender,SuspendingEventArgs ^args ){
//	Print( "Suspending" );
}
 
//WP8 only?
void Win8Game::OnResuming( Platform::Object ^sender,Platform::Object ^args ){
//	Print( "Resuming" );
}

void Win8Game::OnAccelerometerReadingChanged( Accelerometer ^sender,AccelerometerReadingChangedEventArgs ^args ){
	AccelerometerReading ^reading=args->Reading;

	float x=reading->AccelerationX;
	float y=reading->AccelerationY;
	float z=reading->AccelerationZ;
}

#if WINDOWS_PHONE_8
void Win8Game::OnInputPaneShowing( Windows::UI::ViewManagement::InputPane ^sender,Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^args ){
}

//The only way to detect if inputPane has been dismissed...
void Win8Game::OnInputPaneHiding( Windows::UI::ViewManagement::InputPane ^sender,Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^args ){
	if( BBWin8Game::Win8Game()->KeyboardEnabled() ){
		BBWin8Game::Win8Game()->KeyEvent( BBGameEvent::KeyChar,27 );
	}
}

void Win8Game::OnBackButtonPressed( Platform::Object ^sender,Windows::Phone::UI::Input::BackPressedEventArgs ^args ){
	try{
		BBWin8Game::Win8Game()->KeyEvent( BBGameEvent::KeyDown,0x1a0 );
		BBWin8Game::Win8Game()->KeyEvent( BBGameEvent::KeyUp,0x1a0 );
		args->Handled=true;
	}catch( BBExitApp ){
	}
}
#endif
