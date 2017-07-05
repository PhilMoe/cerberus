
'Ok, looks a bit crap and doesn't make much sense, but it's just a test of offset, pitch, byte order etc...
'
Import mojo

Class MyApp Extends App

	Field img:Image
	
	Method OnCreate()
	
		SetUpdateRate 60
		
	End
	
	Field fullscreen:=False
	
	Method OnUpdate()
	
#If TARGET="glfw"
		If KeyHit( KEY_SPACE )
			fullscreen=Not fullscreen
			If fullscreen
				GlfwGame.GetGlfwGame().SetGlfwWindow( 1024,768,8,8,8,0,0,0,True )
			Else
				GlfwGame.GetGlfwGame().SetGlfwWindow( 640,480,8,8,8,0,0,0,False )
			Endif
		End
#Endif
	end
	
	Method OnRender()
	
		If Not img
			Local w=256,h=256
		
			img=CreateImage(w,h,1,Image.MidHandle)
			
			Cls 128,0,255
			SetColor 255,128,0	'Orange!
			DrawRect 16,16,w-32,h-32
			SetColor 255,255,255
			DrawText "Hello World",w/2,h/2,.5,.5

			Local buf:=New Int[w*2*h]
			ReadPixels buf,0,0,w,h ,w,w*2

			For Local i:=0 Until w*h*2
				If i<w*h*2/3
					buf[i]|=$ff0000
				Else If i<w*h*4/3
					buf[i]|=$00ff00
				Else
					buf[i]|=$0000ff
				Endif
			Next
			
			For Local i=0 Until h
				For Local j=0 Until w*2
					buf[i*w*2+j]=buf[i*w*2+j] & $ffffff | (i*255/h) Shl 24
				Next
			Next
			
			img.WritePixels buf,0,0,w,h ,w,w*2
		End
		
		Cls
		Scale 2,2
		SetColor 255,255,255
		DrawImage img,MouseX/2.0,MouseY/2.0
		DrawText "Testing...",0,0
	End
	
End

Function Main()
	New MyApp
End
