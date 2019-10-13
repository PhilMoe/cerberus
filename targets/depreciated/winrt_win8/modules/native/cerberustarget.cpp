
// ***** cerberusgame.h *****

// This class is the bridge to native project, ie: the only class that can see Direct3DBackground class.
//
class BBCerberusGame : public BBWinrtGame{
public:
	BBCerberusGame();
	
	//BBCerberusGame...
	IDXGISwapChain1 *GetSwapChain(){ return _swapChain.Get(); }
	void UpdateGameEx();
	void SwapBuffers();
	
	//BBWinrtGame implementations...
	virtual int GetDeviceWidthX(){ return _devWidth; }
	virtual int GetDeviceHeightX(){ return _devHeight; }
	virtual int GetDeviceRotationX();
	
	virtual ID3D11Device1 *GetD3dDevice(){ return _device.Get(); }
	virtual ID3D11DeviceContext1 *GetD3dContext(){ return _context.Get(); }
	virtual ID3D11RenderTargetView *GetRenderTargetView(){ return _view.Get(); }
	
	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *format );
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz );

	virtual void ValidateUpdateTimer();
	
private:

	int _devWidth,_devHeight;
	
	double _updatePeriod,_nextUpdate;
	
	void CreateD3dResources();
	D3D_FEATURE_LEVEL _featureLevel;

	ComPtr<IDXGISwapChain1> _swapChain;
	ComPtr<ID3D11Device1> _device;
	ComPtr<ID3D11DeviceContext1> _context;
	ComPtr<ID3D11RenderTargetView> _view;
	
	IWICImagingFactory *_wicFactory;
};

// ***** cerberusgame.cpp *****
BBCerberusGame::BBCerberusGame():_updatePeriod( 0 ),_nextUpdate( 0 ),_wicFactory( 0 ){

	_devWidth=DipsToPixels( CoreWindow::GetForCurrentThread()->Bounds.Width );
	_devHeight=DipsToPixels( CoreWindow::GetForCurrentThread()->Bounds.Height );
	
	if( _devWidth<_devHeight ) std::swap( _devWidth,_devHeight );

	CreateD3dResources();
}

void BBCerberusGame::CreateD3dResources(){

	int width=_devWidth;
	int height=_devHeight;

	UINT creationFlags=D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	
#ifdef _DEBUG
//	Not on 8.1, thank you very much!
//	creationFlags|=D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[]={
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

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
		
	DXASS( device.As( &_device ) );
	DXASS( context.As( &_context ) );
	
	//create swap chain
	if( _swapChain ){

		DXASS( _swapChain->ResizeBuffers( 2,width,height,DXGI_FORMAT_B8G8R8A8_UNORM,0 ) );

	}else{

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
		ZEROMEM( swapChainDesc );
		swapChainDesc.Width=width;
		swapChainDesc.Height=height;
		swapChainDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo=false;
		swapChainDesc.SampleDesc.Count=1;
		swapChainDesc.SampleDesc.Quality=0;
		swapChainDesc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount=2;
		swapChainDesc.Scaling=DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.Flags=0;

		ComPtr<IDXGIDevice1> dxgiDevice;
		_device.As( &dxgiDevice );
		
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DXASS( dxgiDevice->GetAdapter( &dxgiAdapter ) );

		ComPtr<IDXGIFactory2> dxgiFactory;
		DXASS( dxgiAdapter->GetParent( __uuidof( IDXGIFactory2 ),(void**)&dxgiFactory ) );
	
		DXASS( dxgiFactory->CreateSwapChainForComposition( _device.Get(),&swapChainDesc,0,&_swapChain ) );
		DXASS( dxgiDevice->SetMaximumFrameLatency( 1 ) );
	}
	
	// Create a render target view of the swap chain back buffer.
	//
	ComPtr<ID3D11Texture2D> backBuffer;
	DXASS( _swapChain->GetBuffer( 0,__uuidof( ID3D11Texture2D ),(void**)&backBuffer ) );
	DXASS( _device->CreateRenderTargetView( backBuffer.Get(),0,&_view ) );
}

void BBCerberusGame::ValidateUpdateTimer(){
	if( _updateRate ){
		_updatePeriod=1.0/_updateRate;
		_nextUpdate=0;
	}
}

void BBCerberusGame::UpdateGameEx(){
	if( _suspended ) return;
	
	//poll accelerometer
	static Windows::Devices::Sensors::Accelerometer ^accel;
	if( !accel ) accel=Windows::Devices::Sensors::Accelerometer::GetDefault();
	if( accel ){
		auto r=accel->GetCurrentReading();
		float x= float( r->AccelerationX );
		float y=-float( r->AccelerationY );
		float z= float( r->AccelerationZ );
		float tx=x,ty=y;
		switch( GetDeviceRotationX() ){
		case 0:
			break;
		case 1:	//Note! Different from winphone 8!
			x=-ty;
			y=tx;
			break;
		case 2:
			x=-tx;
			y=-ty;
			break;
		case 3:	//Note! Different from winphone 8!
			x=ty;
			y=-tx;
			break;
		}
		MotionEvent( BBGameEvent::MotionAccel,-1,x,y,z );
	}
	
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

void BBCerberusGame::SwapBuffers(){
	DXASS( _swapChain->Present( 1,0 ) );
}

int BBCerberusGame::GetDeviceRotationX(){
	switch( DisplayProperties::CurrentOrientation ){
	case DisplayOrientations::Landscape:return 0;
	case DisplayOrientations::Portrait:return 1;
	case DisplayOrientations::LandscapeFlipped:return 2;
	case DisplayOrientations::PortraitFlipped:return 3;
	}
	return 0;
}

unsigned char *BBCerberusGame::LoadImageData( String path,int *pwidth,int *pheight,int *pformat ){

	if( !_wicFactory ){
		DXASS( CoCreateInstance( CLSID_WICImagingFactory,0,CLSCTX_INPROC_SERVER,__uuidof(IWICImagingFactory),(LPVOID*)&_wicFactory ) );
	}
	
	path=PathToFilePath( path );

	IWICBitmapDecoder *decoder;
	if( !SUCCEEDED( _wicFactory->CreateDecoderFromFilename( path.ToCString<wchar_t>(),NULL,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder ) ) ){
		return 0;
	}

	unsigned char *data=0;
	UINT width,height,format=0;
	
	IWICBitmapFrameDecode *bitmapFrame;
	DXASS( decoder->GetFrame( 0,&bitmapFrame ) );
	
	WICPixelFormatGUID pixelFormat;
	DXASS( bitmapFrame->GetSize( &width,&height ) );
	DXASS( bitmapFrame->GetPixelFormat( &pixelFormat ) );
			
	if( pixelFormat==GUID_WICPixelFormat24bppBGR ){
		format=3;
	}else if( pixelFormat==GUID_WICPixelFormat32bppBGRA ){
		format=4;
	}
	
	if( format ){
		data=(unsigned char*)malloc( width*height*format );
		DXASS( bitmapFrame->CopyPixels( 0,width*format,width*height*format,data ) );
		for( unsigned char *t=data;t<data+width*height*format;t+=format ) std::swap( t[0],t[2] );
		*pwidth=width;
		*pheight=height;
		*pformat=format;
	}

	bitmapFrame->Release();
	decoder->Release();
	
	if( data ) gc_ext_malloced( (*pwidth)*(*pheight)*(*pformat) );

	return data;
}

unsigned char *BBCerberusGame::LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){

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
	
	if( data ) gc_ext_malloced( (*length)*(*channels)*(*format) );
	
	return data;
}	
