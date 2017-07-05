
//***** winrtgame.h *****

class BBWinrtGame : public BBGame{
public:
	BBWinrtGame();
	
	static BBWinrtGame *WinrtGame(){ return _winrtGame; }
	
	virtual int GetDeviceWidthX()=0;
	virtual int GetDeviceHeightX()=0;
	virtual int GetDeviceRotationX()=0;
	
	virtual ID3D11Device1 *GetD3dDevice()=0;
	virtual ID3D11DeviceContext1 *GetD3dContext()=0;
	virtual ID3D11RenderTargetView *GetRenderTargetView()=0;

	virtual void ValidateUpdateTimer()=0;

	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *format )=0;
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz )=0;

	virtual int GetDeviceWidth();
	virtual int GetDeviceHeight();
	virtual void SetUpdateRate( int updateRate );
	virtual int Millisecs();
	virtual int SaveState( String state );
	virtual String LoadState();
	virtual bool PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons );
	virtual void OpenUrl( String url );
	
	virtual String PathToFilePath( String path );

	virtual void SuspendGame();
	virtual void ResumeGame();
	
	virtual void OnPointerPressed( Windows::UI::Input::PointerPoint ^p );
	virtual void OnPointerReleased( Windows::UI::Input::PointerPoint ^p );
	virtual void OnPointerMoved( Windows::UI::Input::PointerPoint ^p );

	void Sleep( double time );
	double GetTime();
	
private:
	static BBWinrtGame *_winrtGame;
	
	unsigned int _pointerIds[32];
};

//***** winrtgame.cpp *****

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

BBWinrtGame *BBWinrtGame::_winrtGame;

BBWinrtGame::BBWinrtGame(){

	_winrtGame=this;
	
	memset( _pointerIds,0,sizeof( _pointerIds ) );
}

int BBWinrtGame::GetDeviceWidth(){
	return (GetDeviceRotationX() & 1) ? GetDeviceHeightX() : GetDeviceWidthX();
}

int BBWinrtGame::GetDeviceHeight(){
	return (GetDeviceRotationX() & 1) ? GetDeviceWidthX() : GetDeviceHeightX();
}

void BBWinrtGame::SetUpdateRate( int hertz ){
	BBGame::SetUpdateRate( hertz );
	ValidateUpdateTimer();
}

int BBWinrtGame::Millisecs(){
	return int( GetTime()*1000.0 );
}

int BBWinrtGame::SaveState( String state ){
	if( FILE *f=OpenFile( "cerberus://internal/.cerberusstate","wb" ) ){
		bool ok=state.Save( f );
		fclose( f );
		return ok ? 0 : -2;
	}
	return -1;
}

String BBWinrtGame::LoadState(){
	if( FILE *f=OpenFile( "cerberus://internal/.cerberusstate","rb" ) ){
		String str=String::Load( f );
		fclose( f );
		return str;
	}
	return "";
}

bool BBWinrtGame::PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons ){
	return false;
}

void BBWinrtGame::OpenUrl( String url ){
	auto str=ref new Platform::String( url.ToCString<char16>(),url.Length() );
	auto uri=ref new Windows::Foundation::Uri( str );
	Windows::System::Launcher::LaunchUriAsync( uri );
}

String BBWinrtGame::PathToFilePath( String path ){
	String fpath;
	if( !path.StartsWith( "cerberus:" ) ){
		fpath=path;
	}else if( path.StartsWith( "cerberus://data/" ) ){
		auto folder=Windows::ApplicationModel::Package::Current->InstalledLocation;
		fpath=String( folder->Path )+"/Assets/cerberus/"+path.Slice( 16 );
	}else if( path.StartsWith( "cerberus://internal/" ) ){
		auto folder=Windows::Storage::ApplicationData::Current->LocalFolder;
		fpath=String( folder->Path )+"/"+path.Slice( 20 );
	}
	return fpath;
}

void BBWinrtGame::SuspendGame(){
	BBGame::SuspendGame();
	ValidateUpdateTimer();
}

void BBWinrtGame::ResumeGame(){
	BBGame::ResumeGame();
	ValidateUpdateTimer();
}

double BBWinrtGame::GetTime(){
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

void BBWinrtGame::Sleep( double time ){
	if( WaitForSingleObjectEx( GetCurrentThread(),int( time*1000.0 ),FALSE )==WAIT_OBJECT_0 ){
		//success!
	}
}

void BBWinrtGame::OnPointerPressed( PointerPoint ^p ){
	
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
			if( id>=32 ) return;	//Error! Too many fingers!
			_pointerIds[id]=p->PointerId;
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			TouchEvent( BBGameEvent::TouchDown,id,x,y );
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		{
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			switch( p->Properties->PointerUpdateKind ){
			case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
				MouseEvent( BBGameEvent::MouseDown,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
				MouseEvent( BBGameEvent::MouseDown,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
				MouseEvent( BBGameEvent::MouseDown,2,x,y );
				break;
			}
		}
		break;
	}
}

void BBWinrtGame::OnPointerReleased( PointerPoint ^p ){

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
			if( id>=32 ) return; 	//Pointer ID not found!
			_pointerIds[id]=0;
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			TouchEvent( BBGameEvent::TouchUp,id,x,y );
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		{
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			switch( p->Properties->PointerUpdateKind ){
			case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
				MouseEvent( BBGameEvent::MouseUp,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
				MouseEvent( BBGameEvent::MouseUp,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
				MouseEvent( BBGameEvent::MouseUp,2,x,y );
				break;
			}
		}
		break;
	}
}

void BBWinrtGame::OnPointerMoved( PointerPoint ^p ){

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
			if( id>=32 ) return;	 //Pointer ID not found!
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			TouchEvent( BBGameEvent::TouchMove,id,x,y );
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		{
			float x=DipsToPixels( p->Position.X );
			float y=DipsToPixels( p->Position.Y );
			switch( p->Properties->PointerUpdateKind ){
			case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
				MouseEvent( BBGameEvent::MouseDown,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
				MouseEvent( BBGameEvent::MouseDown,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
				MouseEvent( BBGameEvent::MouseDown,2,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
				MouseEvent( BBGameEvent::MouseUp,0,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
				MouseEvent( BBGameEvent::MouseUp,1,x,y );
				break;
			case Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
				MouseEvent( BBGameEvent::MouseUp,2,x,y );
				break;
			default:
				MouseEvent( BBGameEvent::MouseMove,-1,x,y );
			}
		}
		break;
	case Windows::Devices::Input::PointerDeviceType::Pen:{
		}
		break;
	}
}
