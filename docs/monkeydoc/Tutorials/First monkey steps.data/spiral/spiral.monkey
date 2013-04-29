Import mojo

Class Game Extends App

	' radial sprial with axis aligned phase
		
	Function DrawSpiral(clock)
		Local w=DeviceWidth/2
		For Local i#=0 Until w*1.5 Step .2
			Local x#,y#
			x=w+i*Sin(i*3+clock)
			y=w+i*Cos(i*2+clock)
			DrawPoint x,y
		Next
	End

	Field updateCount
	
	Method OnCreate()
		Print "spiral"
		
		SetUpdateRate 60
	End
	
	Method OnUpdate()
		updateCount+=1
	End
		
	Method OnRender()
		Cls
		DrawSpiral updateCount
		DrawSpiral updateCount*1.1
	End
	
	Method OnSuspend()
		Print "Suspend!"
	End
	
End

Function Main()
	New Game()
End