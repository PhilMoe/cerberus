
' Module mojo.app
'
' Copyright 2011 Mark Sibly, all rights reserved.
' No warranty implied; use at your own risk.

#If MOJO_VERSION_X
Import mojox.app
#Else

Private

Import graphicsdevice
Import audiodevice
Import inputdevice

Import graphics
Import audio
Import input

Import data

Global _app:App
Global _game:=BBGame.Game()
Global _delegate:GameDelegate
Global _updateRate:Int

Class GameDelegate Extends BBGameDelegate

	Field _graphics:GraphicsDevice
	Field _audio:AudioDevice
	Field _input:InputDevice
	
	'***** BBGameDelegate *****
	
	Method StartGame:Void()
		_graphics=New GraphicsDevice
		graphics.SetGraphicsDevice _graphics
		graphics.SetFont Null

		_audio=New AudioDevice
		audio.SetAudioDevice _audio

		_input=New InputDevice
		input.SetInputDevice _input
		
		_app.OnCreate()
	End

	Method SuspendGame:Void()
		_app.OnSuspend()
		_audio.Suspend
	End

	Method ResumeGame:Void()
		_audio.Resume
		_app.OnResume()
	End

	Method UpdateGame:Void()
		_input.BeginUpdate
		_app.OnUpdate()
		_input.EndUpdate
	End

	Method RenderGame:Void()
		Local mode:=_graphics.BeginRender()
		If mode graphics.BeginRender
		If mode=2 _app.OnLoading Else _app.OnRender
		If mode graphics.EndRender
		_graphics.EndRender
	End

	Method KeyEvent:Void( event:Int,data:Int )
		_input.KeyEvent event,data
		If event<>BBGameEvent.KeyDown Return
		Select data
		Case KEY_CLOSE
			_app.OnClose
		Case KEY_BACK
			_app.OnBack
		End
	End

	Method MouseEvent:Void( event:Int,data:Int,x:Float,y:Float )
		_input.MouseEvent event,data,x,y
	End
	
	Method TouchEvent:Void( event:Int,data:Int,x:Float,y:Float )
		_input.TouchEvent event,data,x,y
	End
	
	Method MotionEvent:Void( event:Int,data:Int,x:Float,y:Float,z:Float )
		_input.MotionEvent event,data,x,y,z
	End
	
	Method DiscardGraphics:Void()
		_graphics.DiscardGraphics
	End

End

Public

Class App

	Method New()
		If _app Error "App has already been created"
		_app=Self
		_delegate=New GameDelegate
		_game.SetDelegate _delegate
	End

	Method OnCreate()
	End
	
	Method OnUpdate()
	End
	
	Method OnRender()
	End
	
	Method OnLoading()
	End
	
	Method OnSuspend()
	End
	
	Method OnResume()
	End
	
	Method OnClose()
		EndApp
	End

	Method OnBack()
		OnClose
	End
	
End

Function LoadState$()
	Return _game.LoadState()
End

Function SaveState( state$ )
	Return _game.SaveState( state )
End

Function LoadString$( path$ )
	Return _game.LoadString( FixDataPath(path) )
End

Function SetUpdateRate( hertz )
	_updateRate=hertz
	_game.SetUpdateRate hertz
End

Function UpdateRate()
	Return _updateRate
End

Function Millisecs()
	Return _game.Millisecs()
End

Function GetDate:Int[]()
	Local date:Int[7]
	GetDate date
	Return date
End

Function GetDate( date:Int[] )
	_game.GetDate date
End

Function OpenUrl( url:String )
	_game.OpenUrl url
End

Function HideMouse()
	_game.SetMouseVisible False
End

Function ShowMouse()
	_game.SetMouseVisible True
End

Function EndApp()
	Error ""
End

#Endif
