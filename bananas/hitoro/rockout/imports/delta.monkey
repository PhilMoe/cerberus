
Import rockout

Const TEMP_AVG:Int = 10
	
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
	
	Field average:Float [TEMP_AVG]
	
	Method New (fps:Float)
		targetfps = fps
		lastticks = Millisecs
	End
	
	Method UpdateDelta ()
		currentticks = Millisecs
		frametime = currentticks - lastticks
		
		Local total:Float
		
		For Local loop:Int = 1 Until TEMP_AVG
			average [loop - 1] = average [loop]
			total = total + average [loop - 1]
		Next
		
		average [TEMP_AVG - 1] = currentticks - lastticks
		total = total + average [TEMP_AVG - 1]
		
		frametime = total / Float (TEMP_AVG - 1)
		
		delta = frametime / (1000.0 / targetfps)

		lastticks = currentticks
	End
	
End
