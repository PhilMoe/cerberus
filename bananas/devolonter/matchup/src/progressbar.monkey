Strict

Import mojo

Class ProgressBar
	
Private

	Field x:Float, y:Float
	
	Field width:Float, height:Float
	
	Field value:Float
	
Public

	Method New(x:Float, y:Float, width:Float, height:Float)
		Self.x = x; Self.y = y
		Self.width = width; Self.height = height
		value = 1
	End Method
	
	Method Draw:Void()
		SetColor(255, 107, 107)
		SetAlpha(0.5)
		DrawRect(x, y, width, height)
		SetAlpha(1)
		DrawRect(x, y, width * value, height)
		SetColor(255, 255, 255)
	End Method
	
	Method Value:Void(value:Float) Property
		If (value > 1) value = 1
		Self.value = value
	End Method

End Class