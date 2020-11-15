#pragma once

class BBCerberusGame;

namespace CerberusGameNative{

	public ref class NativeGame sealed{

	public:

		NativeGame( Windows::UI::Xaml::Controls::SwapChainBackgroundPanel ^swapChainPanel );

	private:
	
		BBCerberusGame *_game;
		Windows::UI::Xaml::Controls::SwapChainBackgroundPanel ^_swapChainPanel;
		
		void OnVisibilityChanged( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::VisibilityChangedEventArgs ^args );
		void OnPointerPressed( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::PointerEventArgs ^args );
		void OnPointerMoved( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::PointerEventArgs ^args );
		void OnPointerReleased( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::PointerEventArgs ^args );
		void OnCompositionTargetRendering( Object ^sender,Object ^e );
		void OnOrientationChanged( Platform::Object ^sender );
	};
}
