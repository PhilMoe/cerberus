
'Simple graphics compatibility test
'
'Should behave the same on all targets

Import mojo

Class Test Extends App

	Field image:Image
	
	Field tx#,ty#

	Field c=7,r=255,g=255,b=255
	
	Method OnCreate()

		LoadStuff
		
		SetUpdateRate 60
	End
	
	Method LoadStuff()
		image=LoadImage( "images/RedbrushAlpha.png",1,Image.MidHandle )
	End
	
	Method OnUpdate()

		'Enable to test image thrashing!
#If TARGET<>"html5"
		image.Discard	'should work with/without
		LoadStuff
#End

#rem
		If KeyDown( KEY_RIGHT )
			tx+=.0125
		Else If KeyDown( KEY_LEFT )
			tx-=.0125
		Endif
		If KeyDown( KEY_UP )
			ty-=.0125
		Else If KeyDown( KEY_DOWN )
			ty+=.0125
		Endif
#End
		If KeyHit( KEY_LMB )
			c+=1
			If c=8 c=1
			r=(c&1)* 255
			g=(c&2) Shr 1 * 255
			b=(c&4) Shr 2 * 255
		Endif

	End

	Method OnRender()
	
		Translate tx,ty
		
		Cls 0,0,128
	
		Local sz#=Sin(Millisecs*.1)*32
		Local sx=32+sz,sy=32,sw=DeviceWidth-(64+sz*2),sh=DeviceHeight-(64+sz)
		
		SetScissor sx,sy,sw,sh
		
		Cls 255,32,0
		
		PushMatrix
		
		Scale DeviceWidth/640.0,DeviceHeight/480.0

		Translate 320,240
		Rotate Millisecs/1000.0*12
		Translate -320,-240
		
		SetColor 128,255,0
		DrawRect 32,32,640-64,480-64

		SetColor 255,255,0
		For Local y=0 Until 480
			For Local x=16 Until 640 Step 32
				SetAlpha Min( Abs( y-240.0 )/120.0,1.0 )
				DrawPoint x,y
			Next
		Next

		SetColor 0,128,255
		DrawOval 64,64,640-128,480-128

		SetColor 255,0,128
		DrawLine 32,32,640-32,480-32
		DrawLine 640-32,32,32,480-32

		SetColor r,g,b
		SetAlpha Sin(Millisecs*.3)*.5+.5
		DrawImage image,320,240,0
		SetAlpha 1

		SetColor 255,0,128
		DrawPoly( [ 160.0,232.0, 320.0,224.0, 480.0,232.0, 480.0,248.0, 320.0,256.0, 160.0,248.0 ] )
				
		SetColor 128,128,128
		DrawText "The Quick Brown Fox Jumps Over The Lazy Dog",320,240,.5,.5
		
		PopMatrix
		
		SetScissor 0,0,DeviceWidth,DeviceHeight
		SetColor 128,0,0
		DrawRect 0,0,DeviceWidth,1
		DrawRect DeviceWidth-1,0,1,DeviceHeight
		DrawRect 0,DeviceHeight-1,DeviceWidth,1
		DrawRect 0,0,1,DeviceHeight-1
	End

End

Function Main()

	New Test

End
