
' SIMPLE DEMO: Use LEFT/RIGHT cursors to zoom...

Import mojo.app
Import mojo.graphics
Import mojo.input

Import autofit

' App class...

Class Game Extends App

	Method OnCreate ()
		SetVirtualDisplay 1440, 900				' Required: set virtual display size
		SetUpdateRate 60
	End
	
	Method OnUpdate ()
	
		If KeyDown (KEY_LEFT) Then AdjustVirtualZoom -0.01	' Zoom out
		If KeyDown (KEY_RIGHT) Then AdjustVirtualZoom 0.01	' Zoom in

	End
	
	Method OnRender ()
		
		UpdateVirtualDisplay					' Required: update the virtual display
		
		Cls 128, 128, 128

		' Draw a rectangle in the middle of the virtual display...
		
		SetColor 0, 0, 0		
		DrawRect VDeviceWidth * 0.5, VDeviceHeight * 0.5, 64, 64

		' Draw a rectangle at the virtual mouse position...
		
		SetColor 255, 255, 255	
		DrawRect VMouseX, VMouseY, 64, 64
				
	End
	
End

' Main function...

Function Main ()
	New Game
End

