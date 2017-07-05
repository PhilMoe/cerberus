
Import mojo

Import autofit

' -----------------------------------------------------------------------------
' Demo...
' -----------------------------------------------------------------------------

' All drawing can be hard-coded for a given size (1440 x 900 in this demo),
' but will automatically scale as required to the current device size (which
' is 640 x 480 in this demo). Nice!

Class Game Extends App

	Method OnCreate ()
		SetVirtualDisplay 320,480	'1440, 900
		SetUpdateRate 60
	End

	Method OnUpdate ()

		If KeyDown (KEY_RIGHT) Then AdjustVirtualZoom 0.01
		If KeyDown (KEY_LEFT) Then AdjustVirtualZoom -0.01
		If KeyDown (KEY_ENTER) Then SetVirtualZoom 1.0
		
	End

	Method OnRender ()
		
		UpdateVirtualDisplay
		
		Cls 32, 32, 32
		
		' Corners...
		
		SetColor 255, 0, 0
		DrawRect 0, 0, 32, 32

		SetColor 0, 255, 0
		DrawRect (VDeviceWidth - 1) - 32, 0, 32, 32

		SetColor 0, 0, 255
		DrawRect 0, (VDeviceHeight - 1) - 32, 32, 32

		SetColor 255, 0, 0
		DrawRect (VDeviceWidth - 1) - 32, (VDeviceHeight - 1) - 32, 32, 32
		
		' Borders...
		
		SetColor 255, 0, 0
		DrawLine 0, 0, VDeviceWidth - 1, 0
		SetColor 0, 255, 0
		DrawLine 0, VDeviceHeight - 1, VDeviceWidth - 1, VDeviceHeight - 1
		SetColor 0, 0, 255
		DrawLine 0, 0, 0, VDeviceHeight - 1
		SetColor 255, 255, 0
		DrawLine VDeviceWidth - 1, 0, VDeviceWidth - 1, VDeviceHeight - 1

		' Centre/center...
		
		SetColor 32, 64, 128
		DrawRect (VDeviceWidth / 2.0) - 18, (VDeviceHeight / 2.0) - 18, 36, 36

		' Mouse...
		
		SetColor 255, 255, 255
		DrawRect VMouseX () - 16, VMouseY () - 16, 32, 32

		' Info...
		
		Scale 4, 4
		
		' Note that positions are all multiplied by the scale factor above - had
		' to do this to make things readable!
		
		DrawText "Use LEFT/RIGHT and ENTER to zoom: " + GetVirtualZoom, 20, 20

		DrawText "Device size: " + DeviceWidth + " x " + DeviceHeight, 20, 60
		DrawText "Virtual device size: " + VDeviceWidth + " x " + VDeviceHeight, 20, 80

		DrawText "Virtual mouse co-ords: " + Int (VMouseX) + " x " + Int (VMouseY), 20, 200
		
	End
	
End

Function Main ()
	New Game
End

