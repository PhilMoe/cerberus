Strict

Import mojo
Import gamefield

Public

Class Game Extends App

	Const TIME_OVER:String = "TIME IS UP!"
	
	Const PRESS_SPACE:String = "PRESS SPACE"

	Global Font:BitmapFont

	Field gameField:GameField
	
	Field timeOverWidth:Float
	
	Field pressSpaceWidth:Float
	
	Method OnCreate:Int()
		SetUpdateRate(60)
		
		Font = New BitmapFont("fonts/main.txt")
		timeOverWidth = Font.GetTxtWidth(TIME_OVER)
		pressSpaceWidth = Font.GetTxtWidth(PRESS_SPACE)
		
		gameField = New GameField(4, 3)
		Return 0
	End Method
	
	Method OnUpdate:Int()
		#If TARGET <> "android"
			If (KeyHit(KEY_CLOSE)) Error ""
		#Else
			If (KeyHit(KEY_BACK)) Error ""
		#End		
	
		If (gameField.IsComplete) Then
			If (KeyHit(KEY_SPACE)) Then
				gameField = New GameField(4, 3)
			End If
		Else
			gameField.Update()
		End If
				
		Return 0
	End Method
	
	Method OnRender:Int()
		Cls(0, 0, 0)
		gameField.Draw()
		
		If (gameField.IsComplete) Then
			SetAlpha(0.75)
			SetColor(0, 0, 0)
			DrawRect(0, 0, DeviceWidth(), DeviceHeight())
			SetAlpha(1)
			SetColor(255, 255, 255)
			
			Font.DrawText(TIME_OVER, (DeviceWidth() -timeOverWidth) * 0.5, 170)
			Font.DrawText(PRESS_SPACE, (DeviceWidth() -pressSpaceWidth) * 0.5, 240)
		End If
		Return 0
	End Method

End Class