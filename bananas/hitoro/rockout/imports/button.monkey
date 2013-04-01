
Import rockout

Class Button

	Field x:Int
	Field y:Int
	
	Field width:Int
	Field height:Int
	
	Field text:String
	
	Method New (x:Float, y:Float, str:String)
	
		Self.width = str.Length * FONT_WIDTH
		Self.height = FONT_HEIGHT * 2

		Self.x = x - Self.width * 0.5
		Self.y = y - Self.height * 0.5

		Self.text = str
		
	End
	
	Method Draw ()
	
		SetColor 255, 255, 255
		DrawText text, x, y
		
	End
	
	Method Clicked (clickx:Int, clicky:Int)
		If PointInRect (clickx, clicky, x, y, x + width, y + height)
			If KeyHit (KEY_LMB)
				Return True
			Endif
		Endif
	End
	
End
