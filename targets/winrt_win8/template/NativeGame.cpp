// Class1.cpp
#include "pch.h"
#include "NativeGame.h"

using namespace CerberusGameNative;

using namespace Microsoft::WRL;

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::System;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::Devices::Sensors;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

using namespace concurrency;

#include "CerberusGame.cpp"

NativeGame::NativeGame( SwapChainBackgroundPanel ^swapChainPanel ){

	_game=new BBCerberusGame();
	_swapChainPanel=swapChainPanel;
	
	try{
	
		bb_std_main( 0,0 );
		
	}catch(...){
	
		throw "BYE!";
	}
	
	if( !_game->Delegate() ) return;
	
	ComPtr<ISwapChainBackgroundPanelNative> panelNative;
	DXASS( reinterpret_cast<IUnknown*>( _swapChainPanel )->QueryInterface( IID_PPV_ARGS( &panelNative ) ) );
	DXASS( panelNative->SetSwapChain( _game->GetSwapChain() ) );

	CoreWindow::GetForCurrentThread()->VisibilityChanged+=ref new TypedEventHandler<CoreWindow^,VisibilityChangedEventArgs^>( this,&NativeGame::OnVisibilityChanged );
	CoreWindow::GetForCurrentThread()->PointerPressed+=ref new TypedEventHandler<CoreWindow^,PointerEventArgs^>( this,&NativeGame::OnPointerPressed );
	CoreWindow::GetForCurrentThread()->PointerReleased+=ref new TypedEventHandler<CoreWindow^,PointerEventArgs^>( this,&NativeGame::OnPointerReleased );
	CoreWindow::GetForCurrentThread()->PointerMoved+=ref new TypedEventHandler<CoreWindow^,PointerEventArgs^>( this,&NativeGame::OnPointerMoved );
	CompositionTarget::Rendering+=ref new EventHandler<Object^>( this,&NativeGame::OnCompositionTargetRendering );
	DisplayProperties::OrientationChanged+=ref new DisplayPropertiesEventHandler( this,&NativeGame::OnOrientationChanged );

	OnOrientationChanged( this );

	_game->StartGame();
}

void NativeGame::OnVisibilityChanged( CoreWindow ^sender,VisibilityChangedEventArgs ^args ){

	if( args->Visible ){
		_game->ResumeGame();
	}else{
		_game->SuspendGame();
	}
}

void NativeGame::OnPointerPressed( CoreWindow ^sender,PointerEventArgs ^args ){

	_game->OnPointerPressed( args->CurrentPoint );
}

void NativeGame::OnPointerMoved( CoreWindow ^sender,PointerEventArgs ^args ){

	_game->OnPointerMoved( args->CurrentPoint );
}

void NativeGame::OnPointerReleased( CoreWindow ^sender,PointerEventArgs ^args ){

	_game->OnPointerReleased( args->CurrentPoint );
}

void NativeGame::OnCompositionTargetRendering( Platform::Object ^sender,Object ^e ){

	_game->UpdateGameEx();
	_game->RenderGame();
	_game->SwapBuffers();
}

void NativeGame::OnOrientationChanged( Platform::Object ^sender ){

	DXGI_MODE_ROTATION rot=DXGI_MODE_ROTATION_IDENTITY;
	switch( DisplayProperties::CurrentOrientation ){
	case DisplayOrientations::Landscape:rot=DXGI_MODE_ROTATION_IDENTITY;break;
	case DisplayOrientations::Portrait:rot=DXGI_MODE_ROTATION_ROTATE90;break;
	case DisplayOrientations::LandscapeFlipped:rot=DXGI_MODE_ROTATION_ROTATE180;break;
	case DisplayOrientations::PortraitFlipped:rot=DXGI_MODE_ROTATION_ROTATE270;break;
	}

	_game->GetSwapChain()->SetRotation( rot );
}
