
#pragma once

#include "pch.h"

//implemented in win8game.cpp
ref class Win8Game : public Windows::ApplicationModel::Core::IFrameworkView{
internal:
	Win8Game();
	
public:
	// IFrameworkView Methods.
	virtual void Initialize( Windows::ApplicationModel::Core::CoreApplicationView ^applicationView );
	virtual void SetWindow( Windows::UI::Core::CoreWindow ^window );
	virtual void Load( Platform::String ^entryPoint );
	virtual void Run();
	virtual void Uninitialize();

protected:
	// Event Handlers.
	void OnWindowSizeChanged( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::WindowSizeChangedEventArgs ^args );
	void OnLogicalDpiChanged( Platform::Object ^sender );
	void OnActivated( Windows::ApplicationModel::Core::CoreApplicationView ^applicationView,Windows::ApplicationModel::Activation::IActivatedEventArgs ^args );
	void OnSuspending( Platform::Object ^sender,Windows::ApplicationModel::SuspendingEventArgs ^args );
	void OnResuming( Platform::Object ^sender,Platform::Object ^args );
	void OnWindowClosed( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::CoreWindowEventArgs ^args );
	void OnVisibilityChanged( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::VisibilityChangedEventArgs ^args );

	void OnInputEnabled( Windows::UI::Core::CoreWindow ^window,Windows::UI::Core::InputEnabledEventArgs ^args );

	void OnKeyDown( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::KeyEventArgs ^args );
	void OnKeyUp( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::KeyEventArgs ^args );
	void OnCharacterReceived( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::CharacterReceivedEventArgs ^args );

	void OnPointerPressed( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::PointerEventArgs ^args );
	void OnPointerReleased( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::PointerEventArgs ^args );
	void OnPointerMoved( Windows::UI::Core::CoreWindow ^sender,Windows::UI::Core::PointerEventArgs ^args );

	void OnAccelerometerReadingChanged( Windows::Devices::Sensors::Accelerometer ^sender, Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs ^args );

private:
	bool _windowClosed;
	bool _windowVisible;
};

//implemented in monkeytarget.cpp
ref class MonkeyGame sealed : public Win8Game{
public:
};

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
