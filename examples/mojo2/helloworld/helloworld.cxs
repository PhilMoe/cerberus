
'minimal mojo2 app!

Import mojo2

Class MyApp Extends App

	Field canvas:Canvas
	
	Method OnCreate()

		canvas=New Canvas

	End
	
	Method OnRender()
	
		canvas.Clear 0,0,1
		
		canvas.SetBlendMode 3
		canvas.SetColor 0,0,0,.5
		canvas.DrawText "HELLO WORLD!",DeviceWidth/2+2,DeviceHeight/2+2,.5,.5
		
		canvas.SetBlendMode 1
		canvas.SetColor 1,1,0,1
		canvas.DrawText "HELLO WORLD!",DeviceWidth/2,DeviceHeight/2,.5,.5
		
		canvas.Flush
	End
End

Function Main()
	New MyApp
End
