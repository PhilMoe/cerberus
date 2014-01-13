
'#GLFW_SWAP_INTERVAL=0
'#GLFW_WINDOW_WIDTH=1920
'#GLFW_WINDOW_HEIGHT=1080
'#GLFW_WINDOW_FULLSCREEN=True

Import mojo

Class MyApp Extends App

	Field scroll_x:Float
	Field scroll_mod:Float

	Method OnCreate()
	
		If DesktopMode
			Print "Desktop="+DesktopMode.Width+","+DesktopMode.Height
		Endif
		
		For Local mode:=Eachin DisplayModes
			Print mode.Width+","+mode.Height
		Next
		
		SetUpdateRate 0
		
		scroll_mod=DeviceWidth*3
	End
	
	Method OnUpdate()
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