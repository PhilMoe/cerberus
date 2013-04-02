'This demo is tutor from forum.xuroq.co.uk
'with my custom
import mojo

Function Main()
	New MacOsMenu
End

Class MacOsMenu Extends App
	Field MacOsMenu:myObject[1] 'create class for store variable
	Field speed:Float 'movement speed
	Field numOfItem:Int = 8
	Field space:Int = 20
	
	
	Method OnCreate()
		SetUpdateRate 30 'set FPS
		
		'Resize array
		MacOsMenu = MacOsMenu.Resize(8)
		
		'create new myObject
		For Local i:Int = 0 until numOfItem
			MacOsMenu[i] = new myObject
			MacOsMenu[i].mainImage = LoadImage("MacOsMenu.png") 'Store image on main image
			'Set handle
			MacOsMenu[i].mainImage.SetHandle(MacOsMenu[i].mainImage.Width / 2, MacOsMenu[i].mainImage.Height)
			
			'store our value
			MacOsMenu[i].y = 200
		Next
		
	End
	
	Method OnUpdate()
		'check on over
		For Local i:Int = 0 until numOfItem
			If MouseX() >= MacOsMenu[i].x - MacOsMenu[i].mainImage.Width / 2 * MacOsMenu[i].scaleX And MouseX() < MacOsMenu[i].x + MacOsMenu[i].mainImage.Width / 2 * MacOsMenu[i].scaleX And MouseY() >= MacOsMenu[i].y - MacOsMenu[i].mainImage.Height / 2 * MacOsMenu[i].scaleX And MouseY() < MacOsMenu[i].y + MacOsMenu[i].mainImage.Height / 2 * MacOsMenu[i].scaleX Then
				MacOsMenu[i].alpha = 0.5
			Else
				MacOsMenu[i].alpha = 1
			End
		Next
		
		'update object
		affectScale()
		affectSpace()
	End Method

	Method OnRender()
		Cls
		'Draw Image goes here
		For Local i:Int = 0 until numOfItem
			MacOsMenu[i].Draw()
		Next
	End
	
	'scale all object
	Method affectScale()
		for Local i:Int = 0 until numOfItem
			changeScale(MacOsMenu[i])
		next
	End
	
	'space between all object
	Method affectSpace()
		Local w = 0
		for Local i:Int = 0 until numOfItem - 1
			Local neighborLeft:myObject = MacOsMenu[i]
			Local neighborRight:myObject = MacOsMenu[i + 1]
			neighborRight.x = neighborLeft.x + (neighborLeft.mainImage.Width*neighborLeft.scaleX) / 2 + space + (neighborRight.mainImage.Width*neighborLeft.scaleX) / 2 - neighborRight.mainImage.Width/2
			w += MacOsMenu[i].mainImage.Width + space
		next
		
		MacOsMenu[0].x = (DeviceWidth() -w) / 2 + (MacOsMenu[0].mainImage.Width * MacOsMenu[0].scaleX) / 2
	End
	
	'Scale selected object
	Method changeScale(clip:myObject)
		Local x:Float = MouseX()
		Local y:Float = MouseY()
		Local cx:Float = clip.x
		Local cy:Float = clip.y
		Local prox:Float = Sqrt( (x - cx) * (x - cx) + (y - cy) * (y - cy))
		if (prox < 100) Then
			clip.scaleX = (200 - prox) / 100
			clip.scaleY = clip.scaleX
		else
			clip.scaleX = 1
			clip.scaleY = clip.scaleX
		EndIf
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
