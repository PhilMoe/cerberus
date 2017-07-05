
'Very simple accelerometer demo!
'
'You'll probably need a device with an accelerometer for this to work...

#ANDROID_SCREEN_ORIENTATION="user"

Import mojo

Class MyApp Extends App

	Field w,h
	Field x#,y#,vx#,vy#

	Method OnCreate()
		w=DeviceWidth
		h=DeviceHeight
		x=DeviceWidth/2
		y=DeviceHeight/2
		SetUpdateRate 60
	End
	
	Method OnUpdate()
		Local tx#=x,ty#=y
		
		vx+=AccelX*.5
		vy+=AccelY*.5
		
		x=Clamp( x+vx,10.0,Float(DeviceWidth)-10 )
		y=Clamp( y+vy,10.0,Float(DeviceHeight)-10 )
		
		vx=x-tx
		vy=y-ty
	End
	
	Method OnRender()
		Cls
		DrawText "This way is UP!",0,0
		DrawCircle x,y,20
		SetColor 255,0,0
		DrawCircle x,y,10
	End
End

Function Main()
	New MyApp
End
