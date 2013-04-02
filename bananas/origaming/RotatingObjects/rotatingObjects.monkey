'This demo is tutor from Seinia.com
'with my custom
import mojo

Function Main()
	New rotatingObjects
End

Class rotatingObjects Extends App
	Field rotatingObjects:myObject 'create class for store variable
	Field bullet:myObject 'create class for store variable
	
	Method OnCreate()
		SetUpdateRate 30 'set FPS
		
		rotatingObjects = new myObject 'create new myObject
		rotatingObjects.mainImage = LoadImage("rotatingObjects.png") 'Store image on main image
		'Set handle
		rotatingObjects.mainImage.SetHandle(0, rotatingObjects.mainImage.Height / 2)
		'store our value
		rotatingObjects.x = 100
		rotatingObjects.y = 480
		
		bullet = new myObject 'create Bullet myObject
		bullet.mainImage = LoadImage("bullet.png") 'Store image on main image
		'Set handle
		bullet.mainImage.SetHandle(bullet.mainImage.Width / 2, bullet.mainImage.Height / 2)
		'store our value
		bullet.x = 0
		bullet.y = 0
		bullet.visible = False
	End
	
	Method OnUpdate()
		'determine angle, convert to degrees
		Local dx:Float = MouseX() -rotatingObjects.x;
		Local dy:Float = MouseY() -rotatingObjects.y;
		Local cursorAngle:Float = ATan2(dy, dx);
		rotatingObjects.rotation = -cursorAngle;
	
	
		if Not bullet.visible Then
			'Launch the bullet
			if MouseHit() Then
				Local cos:Float = Cos(-rotatingObjects.rotation)
				Local sin:Float = Sin(-rotatingObjects.rotation)
				Local speed:Float = 20
				
				bullet.x = rotatingObjects.x + cos * rotatingObjects.mainImage.Width
				bullet.y = rotatingObjects.y + sin * rotatingObjects.mainImage.Width
				bullet.vx = cos * speed
				bullet.vy = sin * speed
				bullet.visible = True
			EndIf
		Else
			bullet.vy += 0.6
			'rotate bullet
			Local radians:Float = ATan2(bullet.vy, bullet.vx)
			bullet.rotation = -radians
			'move bullet
			bullet.x += bullet.vx
			bullet.y += bullet.vy
			'remove bullet from stage
			if (bullet.y >= DeviceHeight()) or (bullet.x >= DeviceWidth()) then
				bullet.visible = False
			EndIf
		EndIf
	End Method

	Method OnRender()
		Cls
		'Draw image goes Here
		bullet.Draw()
		rotatingObjects.Draw()
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
