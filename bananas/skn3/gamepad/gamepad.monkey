
Import mojo2

Function Main()
	New Game()
End

'app
Class Game Extends App
	Field canvas:Canvas
	
	Field stickLeft:Float[2]
	Field stickRight:Float[2]
	
	Field triggerLeft:float
	Field triggerRight:float
	
	Field buttons:Bool[JOY_MENU + 1]
	
	Method OnCreate()
		SetUpdateRate(60)
		canvas = New Canvas
	End

	Method OnUpdate()

		'Slow! Don't do this every update in real code!
		CountJoysticks( True )
		
		'update all states
		stickLeft[0] = JoyX(0)
		stickLeft[1] = JoyY(0)
		
		stickRight[0] = JoyX(1)
		stickRight[1] = JoyY(1)
		
		'have to do this (Max) otherwise GLFW seems to combine both triggers incorrectly!
		triggerLeft = Max(0.0, JoyZ(0))
		triggerRight = Max(0.0, JoyZ(1))
		
		For Local index:= JOY_A To JOY_MENU
			buttons[index] = JoyHit(index) Or JoyDown(index)
		Next
	End

	Method OnRender()
		canvas.Clear(0, 0, 0)
		
		Local padCenterX:= DeviceWidth() / 2.0
		Local padCenterY:= DeviceHeight() / 2.0
		
		'left stick
		DrawAnalog(canvas, padCenterX - 125, padCenterY - 50, 40, stickLeft[0], -stickLeft[1], buttons[JOY_LSB])
		
		'right stick
		DrawAnalog(canvas, padCenterX + 60, padCenterY + 50, 40, stickRight[0], -stickRight[1], buttons[JOY_RSB])
		
		'dpad
		DrawDPad(canvas, padCenterX - 100, padCenterY + 10, 80, buttons[JOY_UP], buttons[JOY_DOWN], buttons[JOY_LEFT], buttons[JOY_RIGHT])
		
		'left trigger
		DrawAxis(canvas, padCenterX - 165, padCenterY - 150, 80, 20, triggerLeft)
		
		'right trigger
		DrawAxis(canvas, padCenterX + 85, padCenterY - 150, 80, 20, triggerRight)
		
		'left shoulder
		DrawButton(canvas, padCenterX - 165, padCenterY - 120, 80, 20, buttons[JOY_LB])
		
		'right shoulder
		DrawButton(canvas, padCenterX + 85, padCenterY - 120, 80, 20, buttons[JOY_RB])
		
		'buttons
		Local buttonSize:= 30.0
		Local buttonCenterX:= padCenterX + 125
		Local buttonCenterY:= padCenterY - 50
		
		'a
		DrawButton(canvas, buttonCenterX, buttonCenterY + 40 - (buttonSize / 2.0), buttonSize / 2.0, buttons[JOY_A])

		'b
		DrawButton(canvas, buttonCenterX + 40 - (buttonSize / 2.0), buttonCenterY, buttonSize / 2.0, buttons[JOY_B])
				
		'x
		DrawButton(canvas, buttonCenterX - 40 + (buttonSize / 2.0), buttonCenterY, buttonSize / 2.0, buttons[JOY_X])
		
		'y
		DrawButton(canvas, buttonCenterX, buttonCenterY - 40 + (buttonSize / 2.0), buttonSize / 2.0, buttons[JOY_Y])
		
		'start
		DrawButton(canvas, padCenterX + buttonSize, buttonCenterY - (buttonSize / 4.0), buttonSize, buttonSize / 2.0, buttons[JOY_START])
		
		'back
		DrawButton(canvas, padCenterX - buttonSize - buttonSize, buttonCenterY - (buttonSize / 4.0), buttonSize, buttonSize / 2.0, buttons[JOY_BACK])
		
		'menu
		DrawButton(canvas, padCenterX, buttonCenterY, buttonSize * 0.75, buttons[JOY_MENU])
		
		
		canvas.Flush()
	End
End

'functions
Function DrawAxis:Void(canvas:Canvas, x:Float, y:float, width:Float, height:float, axis:float)
	canvas.SetColor(0.5, 0.5, 0.5)
	canvas.DrawRect(x, y, width, height)
		
	Local size:= Min(16.0, Max(2.0, width * 0.1))
	Local offset:= (width / 2.0) + (axis * ( (width - size) / 2.0)) - (size / 2.0)
		
	If offset >= 0 And offset < width
		canvas.SetColor(0.3, 0.3, 0.3)
		canvas.DrawRect(x + offset, y, size, height)
	EndIf
End
	
Function DrawDPad:Void(canvas:Canvas, x:Float, y:Float, size:Float, up:Bool, down:Bool, left:Bool, right:Bool)
	Local buttonSize:= size / 3.0
	Local buttonPadding:= Min(6.0, Max(2.0, size * 0.04))
		
	canvas.SetColor(0.5, 0.5, 0.5)
	canvas.DrawRect(x, y + buttonSize, size, buttonSize)
	canvas.DrawRect(x + buttonSize, y, buttonSize, size)
		
	canvas.SetColor(0.3, 0.3, 0.3)
	canvas.DrawRect(x + buttonSize, y + buttonSize, buttonSize, buttonSize)
		
	'up
	If up
		canvas.SetColor(0.0, 1.0, 0.0)
	Else
		canvas.SetColor(0.3, 0.3, 0.3)
	EndIf
	canvas.DrawRect(x + buttonSize + buttonPadding, y + buttonPadding, buttonSize - buttonPadding - buttonPadding, buttonSize - buttonPadding)
		
	'down
	If down
		canvas.SetColor(0.0, 1.0, 0.0)
	Else
		canvas.SetColor(0.3, 0.3, 0.3)
	EndIf
	canvas.DrawRect(x + buttonSize + buttonPadding, y + size - buttonSize, buttonSize - buttonPadding - buttonPadding, buttonSize - buttonPadding)
		
	'left
	If left
		canvas.SetColor(0.0, 1.0, 0.0)
	Else
		canvas.SetColor(0.3, 0.3, 0.3)
	EndIf
	canvas.DrawRect(x + buttonPadding, y + buttonSize + buttonPadding, buttonSize - buttonPadding, buttonSize - buttonPadding - buttonPadding)
		
	'right
	If right
		canvas.SetColor(0.0, 1.0, 0.0)
	Else
		canvas.SetColor(0.3, 0.3, 0.3)
	EndIf
	canvas.DrawRect(x + size - buttonSize, y + buttonSize + buttonPadding, buttonSize - buttonPadding, buttonSize - buttonPadding - buttonPadding)
End
	
Function DrawAnalog:Void(canvas:Canvas, x:Float, y:Float, radius:Float, axisX:Float, axisY:Float, pressed:Bool)
	Local padding:= Min(8.0, Max(1.0, radius * 0.1))
	
	canvas.SetColor(0.5, 0.5, 0.5)
	canvas.DrawCircle(x, y, radius)
	
	If pressed
		canvas.SetColor(0, 1.0, 0)
		canvas.DrawCircle(x, y, radius - padding)
	EndIf
	
	canvas.SetColor(0.3, 0.3, 0.3)
		
	Local length:= Sqrt( (axisX * axisX) + (axisY * axisY))
	Local size:= Min(19.0, Max(7.0, radius * 0.1))
	Local cursorX:Float
	Local cursorY:Float
		
	If length > 0
		Local ratio:Float
		If Abs(axisX) > Abs(axisY)
			ratio = Abs(axisX / 1.0) / length
		Else
			ratio = Abs(axisY / 1.0) / length
		EndIf
			
		cursorX = axisX * radius * ratio
		cursorY = axisY * radius * ratio
	EndIf
		
	DrawCross(canvas, x + cursorX - (size / 2.0), y + cursorY - (size / 2.0), size, size)
End

Function DrawCross:Void(canvas:Canvas, x:Float, y:Float, width:Float, height:Float, diagonal:Bool = True)
	' --- draw a cross ---
	If diagonal
		canvas.DrawLine(x, y, x + width, y + height)
		canvas.DrawLine(x + width, y, x, y + height)
	Else
		Local halfWidth:= width / 2.0
		Local halfHeight:= height / 2.0
		canvas.DrawLine(x + halfWidth, y, x + halfHeight, y + height)
		canvas.DrawLine(x, y + halfHeight, x + width, y + halfHeight)
	EndIf
End
	
Function DrawButton:Void(canvas:Canvas, x:Float, y:Float, radius:Float, pressed:Bool)
	Local padding:= Min(8.0, Max(1.0, radius * 0.1))
	
	canvas.SetColor(0.5, 0.5, 0.5)
	canvas.DrawCircle(x, y, radius)
	
	If pressed
		canvas.SetColor(0, 1.0, 0)
		canvas.DrawCircle(x, y, radius - padding)
	EndIf
End

Function DrawButton:Void(canvas:Canvas, x:Float, y:Float, width:Float, height:Float, pressed:Bool)
	Local padding:= Min(8.0, Max(1.0, width * 0.05))
	
	canvas.SetColor(0.5, 0.5, 0.5)
	canvas.DrawRect(x, y, width, height)
	
	If pressed
		canvas.SetColor(0, 1.0, 0)
		canvas.DrawRect(x + padding, y + padding, width - padding - padding, height - padding - padding)
	EndIf
End