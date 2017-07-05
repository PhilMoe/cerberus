#include "pch.h"
#include "Direct3DContentProvider.h"
#include "CerberusGame.PhoneComponent.h"

using namespace Microsoft::WRL;
using namespace Windows::Phone::Input::Interop;
using namespace Windows::Phone::Graphics::Interop;

using namespace CerberusGame_PhoneComponent;

#include "CerberusGame.cpp"

Direct3DBackground::Direct3DBackground():_get( 0 ),_put( 0 )
{
	KeyChar=0;

	_game=new BBCerberusGame( this );
	
	try{

		bb_std_main( 0,0 );
		
	}catch(...){
	
		return;
	}
	
	if( !_game->Delegate() ) return;
}

IDrawingSurfaceBackgroundContentProvider^ Direct3DBackground::CreateContentProvider()
{
    ComPtr<Direct3DContentProvider> provider = Make<Direct3DContentProvider>(this);
    return reinterpret_cast<IDrawingSurfaceBackgroundContentProvider^>(provider.Get());
}

void Direct3DBackground::SetManipulationHost(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ manipulationHost)
{
	manipulationHost->PointerPressed +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerPressed);

	manipulationHost->PointerMoved +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerMoved);

	manipulationHost->PointerReleased +=
		ref new TypedEventHandler<DrawingSurfaceManipulationHost^, PointerEventArgs^>(this, &Direct3DBackground::OnPointerReleased);
}

void Direct3DBackground::Application_Activated()
{
	_game->ResumeGame();
}

void Direct3DBackground::Application_Deactivated()
{
	_game->SuspendGame();
}

//return true to cancel
bool Direct3DBackground::OnBackKeyPress()
{
	try
	{
		_game->KeyEvent( BBGameEvent::KeyDown,0x1a0 );
		_game->KeyEvent( BBGameEvent::KeyUp,0x1a0 );
		return true;
	}
	catch( BBExitApp )
	{
	}
	return false;
}

void Direct3DBackground::RunOnUIThread(){

	_game->RunOnUIThread();
}

void Direct3DBackground::FlushEvents(){
	while( _get!=_put ){
		switch( _events[_get].type ){
		case 0:_game->OnPointerPressed( _events[_get].point );break;
		case 1:_game->OnPointerMoved( _events[_get].point );break;
		case 2:_game->OnPointerReleased( _events[_get].point );break;
		}
		_get=(_get+1)&255;
	}
	if( KeyChar ){
		_game->KeyEvent( BBGameEvent::KeyChar,KeyChar );
		KeyChar=0;
	}

}

// Event Handlers
void Direct3DBackground::OnPointerPressed(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	_events[_put].type=0;
	_events[_put].point=args->CurrentPoint;
	_put=(_put+1)&255;
}

void Direct3DBackground::OnPointerMoved(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	_events[_put].type=1;
	_events[_put].point=args->CurrentPoint;
	_put=(_put+1)&255;
}

void Direct3DBackground::OnPointerReleased(DrawingSurfaceManipulationHost^ sender, PointerEventArgs^ args)
{
	_events[_put].type=2;
	_events[_put].point=args->CurrentPoint;
	_put=(_put+1)&255;
}

// Interface With Direct3DContentProvider
HRESULT Direct3DBackground::Connect(_In_ IDrawingSurfaceRuntimeHostNative* host, _In_ ID3D11Device1* device)
{
    return S_OK;
}

void Direct3DBackground::Disconnect()
{
}

HRESULT Direct3DBackground::PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Inout_ DrawingSurfaceSizeF* desiredRenderTargetSize)
{
	desiredRenderTargetSize->width=RenderResolution.Width;
	desiredRenderTargetSize->height=RenderResolution.Height;

    return S_OK;
}

HRESULT Direct3DBackground::Draw(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* view)
{
	_game->UpdateD3dDevice( device,context,view );
	
	_game->StartGame();
	
	FlushEvents();
	
	_game->UpdateGameEx();
	
	_game->RenderGame();
	
	RequestAdditionalFrame();
	
	return S_OK;
}
