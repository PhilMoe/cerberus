#pragma once

#include "pch.h"
#include <DrawingSurfaceNative.h>

class BBCerberusGame;

namespace CerberusGame_PhoneComponent
{
	//Because pointer events appear to happen on another thread...?
	struct PointerEvent{
		int type;
		Windows::UI::Input::PointerPoint ^point;
	};
	
	public delegate void PostToUIThreadHandler();
    public delegate void RequestAdditionalFrameHandler();
	public delegate void ActivateKeyboardHandler( bool activate );

	[Windows::Foundation::Metadata::WebHostHidden]
    public ref class Direct3DBackground sealed : public Windows::Phone::Input::Interop::IDrawingSurfaceManipulationHandler
    {
    public:
        Direct3DBackground();

        Windows::Phone::Graphics::Interop::IDrawingSurfaceBackgroundContentProvider^ CreateContentProvider();
        event RequestAdditionalFrameHandler^ RequestAdditionalFrame;
		
		virtual void SetManipulationHost(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ manipulationHost);

		void Application_Activated();
		void Application_Deactivated();
		
		//return true to cancel
		bool OnBackKeyPress();

		void RunOnUIThread();

		property int DeviceRotation;
		property Windows::Foundation::Size WindowBounds;
		property Windows::Foundation::Size NativeResolution;
		property Windows::Foundation::Size RenderResolution;
		property PostToUIThreadHandler ^PostToUIThread;
		property ActivateKeyboardHandler ^ActivateKeyboard;
		property int KeyChar;

	protected:
	
		void FlushEvents();

		void OnPointerPressed(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);
		void OnPointerReleased(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);
		void OnPointerMoved(Windows::Phone::Input::Interop::DrawingSurfaceManipulationHost^ sender, Windows::UI::Core::PointerEventArgs^ args);

    internal:
        HRESULT Connect(_In_ IDrawingSurfaceRuntimeHostNative* host, _In_ ID3D11Device1* device);
        void Disconnect();

        HRESULT PrepareResources(_In_ const LARGE_INTEGER* presentTargetTime, _Inout_ DrawingSurfaceSizeF* desiredRenderTargetSize);
        HRESULT Draw(_In_ ID3D11Device1* device, _In_ ID3D11DeviceContext1* context, _In_ ID3D11RenderTargetView* renderTargetView);
        
	private:
		BBCerberusGame *_game;
		PointerEvent _events[256];
		int _put,_get;
    };
}
