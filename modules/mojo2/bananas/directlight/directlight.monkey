
Import mojo2

Import mojo2.glutil

Class MyApp Extends App

	Field canvas:Canvas
	Field tile:Image
	
	Method Test()
	End
	
	Method Test2()
			Return Test()
	End
	
	Method OnCreate()
		
		canvas = New Canvas
		
		canvas.SetAmbientLight .2, .2, .2
		
		tile = Image.Load( "t3.png", 0, 0 )
		
	End
	
	Method OnRender()
	
		'If Texture.TexturesLoading Return
		
		canvas.Clear( 0, 0, 1, 1 )
	
		'Set light 0
		canvas.SetLightType 0, 1
		canvas.SetLightColor 0, .3, .3, .3
		canvas.SetLightPosition 0, MouseX, MouseY, -100
		canvas.SetLightRange 0,200
		
		'Light will affect subsequent rendering...
		For Local x:=0 Until DeviceWidth Step 128
			For Local y:=0 Until DeviceHeight Step 128	
				canvas.DrawImage tile, x, y
			Next
		Next
		
		canvas.Flush
		
	End

End

Function Main()

	New MyApp

End

