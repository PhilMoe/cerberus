
Import mojo

Class TransformApp Extends App
	Field t#
	Field mode
	
	Field savedMatrix#[]
	
	Method OnCreate()
		SetUpdateRate 60
		
		PushMatrix
			Translate 10,0
			Rotate 20
			savedMatrix = GetMatrix()		'save a transformation matrix representing a step forward by 10 units and then turn left 20 degrees
		PopMatrix
		
	End
	
	Method OnUpdate()
		t = (t+2) Mod 500
		
		If t =0
			mode = (mode + 1) Mod 3	'cycle through the modes
		Endif
		
		
	End
	
	Method OnRender()
		Cls

		SetColor 255,255,255
		
		If (mode+1) & 1
			DrawText "xShear on",0,0
		Endif
		If (mode+1) & 2
			DrawText "yShear on",0,30
		Endif
		
		PushMatrix

		Translate DeviceWidth/2,DeviceHeight/2		'move to the centre of the screen
	
		If (mode+1) & 1 xShear()
		If (mode+1) & 2 yShear()
		
		SetColor 255,255,255
		DrawRect -50,-50,50,50		'draw a rectangle, in white
		
		SetColor 255,255,0
		DrawLine 0,0,100,0	'draw what would normally be a horizontal line, in yellow
		
		SetColor 0,0,255
		drawpattern	'draw a nice pattern, with the transformations applied, in blue
		
		SetColor 255,0,0
		DrawLine 0,0,0,100	'draw what would normally be a vertical line, in red
		
		PopMatrix
		
	End
	
	Method xShear()
		Transform 1,0,Cos(t),1,0,0	
	End
	
	Method yShear()
		Transform 1,Sin(t),0,1,0,0
	End
	
	Method drawpattern()
		PushMatrix
			For Local i=0 To 10
				DrawRect 0,0,10,10
				Transform savedMatrix[0],savedMatrix[1],savedMatrix[2],savedMatrix[3],savedMatrix[4],savedMatrix[5]	'apply the saved transformation matrix
			Next
		PopMatrix
	End
End

Function Main()
	New TransformApp
End