
' NOTES:

' temp:Button used for now, to make player start in sensible location...

Import rockout

Import imports.session
Import imports.level

Import imports.updategame
Import imports.rendergame
Import imports.checkkeys

' -----------------------------------------------------------------------------
' Extended app definition...
' -----------------------------------------------------------------------------

Const BASE_FPS:Int = 60		' DO NOT CHANGE! All movements based on 60 FPS...

Global UPDATE_RATE:Int = 60		' This can be changed. Recommended minimum
						' 60. Max limited by device CPU/timing --
						' game will run slow if set too high for CPU.

						' Set it too low and you can see how collisions
						' start to fail, as they're not being checked
						' often enough. Higher means more frequent checks.
						
' May pre-calc this instead, but it's nice for realtime FPS change demo...

Function FrameScale:Float (value:Float)
	Return value * 1.0 / (Float (UPDATE_RATE) / Float (BASE_FPS))
End

Class GameApp Extends App

	Field GameWidth:Int
	Field GameHeight:Int
	
	Global FrameScale:Float
	
	Field mx:Float
	Field my:Float
	
	Method New (width:Int, height:Int)
		GameWidth = width
		GameHeight = height
	End

	Field temp:Button
	
	Method OnLoading ()
'		Cls 255, 0, 0
'		DrawRect 32, 32, 32, 32
	End
	
	Method OnCreate ()
	
		SetVirtualDisplay GameWidth, GameHeight
		
		' Move these to session or level loader!
		
		DEFAULT_PLAYER		= LoadCenteredImage (IMAGE_PLAYER)
		DEFAULT_SHOT		= LoadCenteredImage (IMAGE_SHOT)
		DEFAULT_BLOCK		= LoadCenteredImage (IMAGE_BLOCK)

		GameSession = New Session
		GameSession.SetState STATE_MENU
		
		SetUpdateRate UPDATE_RATE
		
		SetFont Null ' TODO: Temp Flash hack! Remove when fixed!
		
			' TEMP!
		
				Local start:String = "Click HERE to START!"
				temp = New Button (VDeviceWidth / 2, VDeviceHeight - 96, start)


	End
	
	Method OnUpdate ()
		UpdateGame
	End

	Method OnRender ()
		RenderStates
	End
	
End
