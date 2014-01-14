
#GLFW_WINDOW_WIDTH=0
#GLFW_WINDOW_HEIGHT=0

Import mojo

Class MyApp Extends App

	Field scroll_x:Float
	Field scroll_mod:Float

	Method OnCreate()
		SetDeviceWindow DesktopMode().Width,DesktopMode().Height,1
		SetSwapInterval 1
		SetUpdateRate 0
		scroll_mod=DeviceWidth*3
	End
	
	Method OnUpdate()
		If KeyHit( KEY_ESCAPE ) EndApp
		scroll_x=(scroll_x+2) Mod scroll_mod
	End
	
	Method OnRender()
		random.Seed=1234
		Cls
		For Local i:=0 Until 100
			Local x:=Rnd( scroll_mod )
			Local y:=Rnd( DeviceHeight )
			Local w:=Rnd( 256 )
			Local h:=Rnd( 256 )
			SetColor Rnd( 256 ),Rnd( 256 ),Rnd( 256 )
			DrawRect x-scroll_x,y,w,h
			If x<DeviceWidth DrawRect x+scroll_mod-scroll_x,y,w,h
			If x+w>scroll_mod DrawRect x-scroll_mod-scroll_x,y,w,h
		Next
		SetColor 0,255,0
		DrawText scroll_x,0,0
	End
	
End

Function Main()
	New MyApp
End