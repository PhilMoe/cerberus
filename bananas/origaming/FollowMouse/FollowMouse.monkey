'This demo is tutor from asgamer.com
'with my custom
import mojo

Function Main()
	New followMouse
End

Class followMouse Extends App
	Field followMouse:myObject 'create class for store variable
	Field speed:Float 'movement speed
	
	Method OnCreate()
		SetUpdateRate 30 'set FPS
		
		followMouse = new myObject 'create new myObject
		followMouse.mainImage = LoadImage("FollowMouse.png") 'Store image on main image
		'Set mid handle
		followMouse.mainImage.SetHandle(followMouse.mainImage.Width / 2, followMouse.mainImage.Height / 2)
		
		'store our value
		followMouse.x = 0
		followMouse.y = 0
		
		'set object speed
		speed = 7
	End
	
	Method OnUpdate()

	End Method

	Method OnRender()
		Cls
		followMouse.x -= (followMouse.x - MouseX()) / speed
		followMouse.y -= (followMouse.y - MouseY()) / speed
		'Draw image goes Here
		followMouse.Draw()
	End
End

class myObject
	Field mainImage:Image
	Field x:Float, y:Float, rotation:Float, vx:Float, vy:Float
	Field alpha:Float = 1
	Field scaleX:Float = 1
	Field scaleY:Float = 1
	Field visible:Bool = True
	
	Method Draw:Void()
		if Not visible Then Return
		SetAlpha alpha
		DrawImage(mainImage, x, y, rotation, scaleX, scaleY)
		SetAlpha 1
	End
End
