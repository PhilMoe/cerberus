
'Simple graphics compatibility test
'
'Should behave the same on all targets

Import mojo2

Class Test Extends App

	Field canvas:Canvas
	Field image:Image
	
	Field tx#,ty#

	Field c=7,r=255,g=255,b=255
	
	Method OnCreate()
	
		canvas=New Canvas

		LoadStuff
		
		SetUpdateRate 0
	End
	
	Method LoadStuff()
		If image image.Discard
		image=Image.Load( "images/RedbrushAlpha.png" )
	End
	
	Method OnUpdate()

		If KeyHit( KEY_LMB )
		
			c+=1
			If c=8 c=1
			r=(c&1)* 255
			g=(c&2) Shr 1 * 255
			b=(c&4) Shr 2 * 255
			
			LoadStuff

		Endif

	End

	Method OnRender()
	
		canvas.SetScissor 0,0,DeviceWidth,DeviceHeight
		canvas.Clear 0,0,.5
	
		Local sz#=Sin(Millisecs*.1)*32
		Local sx=32+sz,sy=32,sw=DeviceWidth-(64+sz*2),sh=DeviceHeight-(64+sz)
		
		canvas.SetScissor sx,sy,sw,sh
		canvas.Clear 1,32.0/255.0,0
		
		canvas.PushMatrix
		canvas.Translate tx,ty
		canvas.Scale DeviceWidth/640.0,DeviceHeight/480.0
		canvas.Translate 320,240
		canvas.Rotate Millisecs/1000.0*12
		canvas.Translate -320,-240
		
		canvas.SetColor .5,1,0
		canvas.DrawRect 32,32,640-64,480-64

		canvas.SetColor 1,1,0
		For Local y=0 Until 480
			For Local x=16 Until 640 Step 32
				canvas.SetAlpha Min( Abs( y-240.0 )/120.0,1.0 )
				canvas.DrawPoint x,y
			Next
		Next
		canvas.SetAlpha 1
		
		canvas.SetColor 0,.5,1
		canvas.DrawOval 64,64,640-128,480-128

		canvas.SetColor 1,0,.5
		canvas.DrawLine 32,32,640-32,480-32
		canvas.DrawLine 640-32,32,32,480-32

		canvas.SetColor r/255.0,g/255.0,b/255.0,Sin(Millisecs*.3)*.5+.5
		canvas.DrawImage image,320,240,0
		canvas.SetAlpha 1

		canvas.SetColor 1,0,.5
		canvas.DrawPoly( [ 140.0,232.0, 320.0,224.0, 500.0,232.0, 500.0,248.0, 320.0,256.0, 140.0,248.0 ] )
				
		canvas.SetColor .5,.5,.5
		canvas.DrawText "The Quick Brown Fox Jumps Over The Lazy Dog",320,240,.5,.5
		
		canvas.PopMatrix
		
		canvas.SetScissor 0,0,DeviceWidth,DeviceHeight
		canvas.SetColor 1,0,0'.5,0,0
		canvas.DrawRect 0,0,DeviceWidth,1
		canvas.DrawRect DeviceWidth-1,0,1,DeviceHeight
		canvas.DrawRect 0,DeviceHeight-1,DeviceWidth,1
		canvas.DrawRect 0,0,1,DeviceHeight-1
		
		canvas.Flush
	End

End

Function Main()

	New Test

End
