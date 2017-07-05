
Import mojo2

Import brl.databuffer

Class MyApp Extends App

	Field canvas:Canvas
	
	Field image:Image
	
	Field data:DataBuffer
	
	Method OnCreate()

		canvas=New Canvas

		image=New Image( 256,256 )
		
		data=New DataBuffer( 16*16*4 )
		
	End
	
	Method OnUpdate()
		If KeyHit( KEY_SPACE ) CrashGraphics
	End
	
	Method OnRender()
	
		'write databuffer to image...
		'
		Local pitch:=256*4
		
		For Local x:=0 Until 16
		
			For Local y:=0 Until 16

				Local r:=1.0,g:=1.0,b:=1.0,a:=1.0
				
				If (x~y)&1
					r=Sin( Millisecs*.1 )*.5+.5
					g=Cos( Millisecs*.1 )*.5+.5
					b=.5
				Endif
				
				Local rgba:=a*255 Shl 24 | b*255 Shl 16 | g*255 Shl 8 | r*255
				
				For Local i:=0 Until 16*16*4 Step 4
					data.PokeInt i,rgba
				Next
				
				image.WritePixels x*16,y*16,16,16,data

			Next
		Next
		
		'render image to main canvas...
		'
		canvas.Clear
		canvas.DrawImage image,MouseX,MouseY
		canvas.Flush
	End
End

Function Main()
	New MyApp
End

		
		
	
	
	