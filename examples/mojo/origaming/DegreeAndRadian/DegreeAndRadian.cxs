import mojo

Function Main()
	New DegreeAndRadian
End

Class DegreeAndRadian Extends App
	Field DegreeAndRadian:myObject 'create class for store variable
	Field angle:Float
	Method OnCreate()
		SetUpdateRate 30 'set FPS
		
		DegreeAndRadian = new myObject 'create new myObject
		DegreeAndRadian.mainImage = LoadImage("DegreeAndRadian.png") 'Store image on main image
		'Set handle
		DegreeAndRadian.mainImage.SetHandle(0, DegreeAndRadian.mainImage.Height / 2)
		'store our value
		DegreeAndRadian.x = DeviceWidth / 2
		DegreeAndRadian.y = DeviceHeight / 2
	End
	
	Method OnUpdate()
		Local theX:Float = MouseX - DegreeAndRadian.x
		Local theY:Float = (MouseY - DegreeAndRadian.y) * -1
		angle = ATan(theY / theX)
		if (theX < 0) then
			angle += 180
		EndIf
		if (theX >= 0 and theY < 0) then
			angle += 360
		EndIf
		'Set the angle
		DegreeAndRadian.rotation = angle
	End Method

	Method OnRender()
		Cls
		'Draw image goes Here
		DrawText("Degree " + angle, 0, 0)
		DrawText("Radian " + (angle * PI / 180), 0, 20)

		DegreeAndRadian.Draw()
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
