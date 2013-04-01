
Import mojo

Class DeltaTimer

	' Usage...
	
	' 1)	Create DeltaTimer object, eg.
	' 		"Local dt:DeltaTimer = New DeltaTimer (60)"
	' 		where 60 is your game's intended frame rate,
	' 		regardless of device frame rate.
	
	' 2)	Call dt.UpdateDelta at start of OnUpdate...
	
	' 3)	Multiply all speeds by dt.delta...
	
	' 4)	That's it.
	
	Field targetfps:Float = 60
	Field currentticks:Float
	Field lastticks:Float
	Field frametime:Float
	Field delta:Float
	
	Method New (fps:Float)
		targetfps = fps
		lastticks = Millisecs
	End
	
	Method UpdateDelta ()
		currentticks = Millisecs
		frametime = currentticks - lastticks
		delta = frametime / (1000.0 / targetfps)
		lastticks = currentticks
	End
	
End

Class Game Extends App

	Global FPS:Int = 60
	
	' Position/speed of example rects...
	
	Field ux:Float = -16			' Position increased by uncorrected speed
	Field dx:Float = -16			' Position increased by DELTA-CORRECTED speed
	Field xs:Float = 4.0			' Speed
	
	Field dt:DeltaTimer				' A handle for the delta timer (to be created in OnCreate)
	
	Method OnCreate ()
		dt = New DeltaTimer ( FPS )	'  Gameplay update rate...
		SetUpdateRate FPS			' ... may be different to device/app update rate!
	End
	
	Method OnUpdate ()

		' ---------------------------------------------------------------------------
		' 1) Get new frame delta...
		' ---------------------------------------------------------------------------
		
		dt.UpdateDelta
		
		' ---------------------------------------------------------------------------
		' 2) Multiply speeds by frame delta...
		' ---------------------------------------------------------------------------

		ux = ux + xs				' Speed NOT scaled by frame delta, ie. how NOT to do it...
		dx = dx + xs * dt.delta		' Speed scaled by frame delta...
		
		' Wrap rects around screen...
		
		If ux > DeviceWidth + 16 Then ux = -16
		If dx > DeviceWidth + 16 Then dx = -16
		
		' Change FPS on the fly to see effect...
		
		If KeyHit (KEY_LEFT)
			FPS = FPS - 10
			If FPS < 10 Then FPS = 10
			SetUpdateRate FPS
		Endif
		
		If KeyHit (KEY_RIGHT)
			FPS = FPS + 10
			SetUpdateRate FPS
		Endif
		
		If KeyDown (KEY_ENTER)
			FPS = 60
			SetUpdateRate FPS
		Endif

	End
	
	Method OnRender ()
	
		Cls 32, 64, 128
		
		DrawText "Time scale factor (Delta): " + dt.delta + " (Frame time: " + dt.frametime + " ms)", 0, 20
		
		SetColor 255, 0, 0
		DrawRect ux - 16, 200 - 16, 32, 32
		SetColor 255, 255, 255
		DrawText "Uncorrected", ux - 16, 200 - 32
		
		SetColor 0, 255, 0
		DrawRect dx - 16, 260 - 16, 32, 32
		SetColor 255, 255, 255
		DrawText "Corrected by delta timing", dx - 16, 260 - 32
		
		DrawText "Use <- and -> plus ENTER to change game update rate (currently " + FPS + " fps)", 0, 60
		
	End
	
End

Function Main ()
	New Game
End
