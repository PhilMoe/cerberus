
Import mojo2

Class MyApp Extends App

	Field canvas:Canvas
	
	Field image:Image
	Field icanvas:Canvas
	
	Method OnCreate()
		canvas=New Canvas

		image=New Image( 256,256 )
		icanvas=New Canvas( image )
		
	End
	
	Method OnRender()
	
		'render to image...
		For Local x:=0 Until 16
			For Local y:=0 Until 16
				If (x~y)&1
					icanvas.SetColor Sin( Millisecs*.1 )*.5+.5,Cos( Millisecs*.1 )*.5+.5,.5
				Else
					icanvas.SetColor 1,1,0
				Endif
				icanvas.DrawRect x*16,y*16,16,16
			Next
		Next
		icanvas.Flush
		
		'render to main canvas...
		canvas.Clear
		canvas.DrawImage image,MouseX,MouseY
		canvas.Flush
	End
End

Function Main()
	New MyApp
End

		
		
	
	
	