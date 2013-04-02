
' -----------------------------------------------------------------------------
' Rotation around player. Confusing, disorientating and, well, sickening...
' -----------------------------------------------------------------------------

' WARNING: MAY INDUCE MOTION SICKNESS! Not actually kidding...

' This looks and feels 'wrong' while moving since the rotation point is also
' constantly moving, but it's doing what it's meant to do.

' Basic imports for most purposes...

Import mojo

' -----------------------------------------------------------------------------
' DISPLAY ROTATION FUNCTIONS...
' -----------------------------------------------------------------------------



' -----------------------------------------------------------------------------
' RotateDisplay: rotates view around x, y by angle...
' -----------------------------------------------------------------------------

	Function RotateDisplay (x:Float, y:Float, angle:Float)
		PushMatrix				' Store current rotation, scale, etc
		Translate x, y			' Shift origin across to here
		Rotate angle			' Rotate around origin
		Translate -x, -y		' Shift origin back where it was
	End

' -----------------------------------------------------------------------------
' ResetDisplay: resets view back to normal...
' -----------------------------------------------------------------------------

	Function ResetDisplay ()
		PopMatrix				' Restore rotation, scale, etc
	End

Class Block
	Field x:Float
	Field y:Float
	Field image:Image
End

' App class...

Class Game Extends App

	Field player:Image
	Field block:Image
	
	Field px:Float
	Field py:Float

	Field pxs:Float
	Field pys:Float

	Field ang:Float
	Field angspeed:Float
	
	Field rotatemode:Int
	
	Field blocklist:List <Block>
	
	Method OnCreate ()		' Load media and SET UPDATE RATE here...

		px = DeviceWidth * 0.5	
		py = DeviceHeight * 0.5	

		player	= LoadCenteredImage ("player.png")
		block		= LoadCenteredImage ("block.png")
		
		blocklist = New List <Block>
		
		For Local loop:Int = 1 To 100
			Local temp:Block = New Block
			temp.image = block
			temp.x = Rnd (-DeviceWidth, DeviceWidth)
			temp.y = Rnd (-DeviceHeight, DeviceHeight)
			blocklist.AddLast temp
		Next
		
		SetUpdateRate 60
		
		rotatemode = 1
		
	End
	
	Method OnUpdate ()		' Take input and update game here...

		If KeyDown (KEY_LEFT) Then pxs = pxs - 0.1
		If KeyDown (KEY_RIGHT) Then pxs = pxs + 0.1

		If KeyDown (KEY_UP) Then pys = pys - 0.1
		If KeyDown (KEY_DOWN) Then pys = pys + 0.1

		If KeyDown (KEY_Z) Then angspeed = angspeed - 0.01
		If KeyDown (KEY_X) Then angspeed = angspeed + 0.01

		If KeyDown (KEY_SPACE)
			pxs = 0
			pys = 0
		Endif

		If KeyDown (KEY_ENTER) Then angspeed = 0
		
		If KeyHit (KEY_R)
		
			rotatemode = rotatemode + 1
			If rotatemode > 4 Then rotatemode = 1
		
			px = DeviceWidth * 0.5	
			py = DeviceHeight * 0.5	

			pxs = 0
			pys = 0
			
			ang = 0
			angspeed = 0
		
		Endif
		
		px = px + pxs
		py = py + pys
		
		If px < 0
			pxs = 0
			px = 0
		Else
			If px > DeviceWidth
				pxs = 0
				px = DeviceWidth - 1
			Endif
		Endif

		If py < 0
			pys = 0
			py = 0
		Else
			If py > DeviceHeight
				pys = 0
				py = DeviceHeight - 1
			Endif
		Endif
		
		ang = ang + angspeed
		' ang = ang Mod 360.0 ' Mod returning Int!

	End
	
	Method OnRender ()		' Draw frame here...
	
		Cls 64, 96, 128

		Select rotatemode
		
			Case 1

				RotateDisplay px, py, ang
				
				SetColor 0, 64, 0
				DrawRect -DeviceWidth, DeviceHeight * 0.5, DeviceWidth * 4.0, DeviceHeight * 3.0 ' Just making sure...
					
				For Local b:Block = Eachin blocklist
					DrawImage b.image, b.x, b.y, 0, 0.25, 0.25
				Next
		
				ResetDisplay

				' Display set to normal so player drawn normally, with control movements
				' relative to screen as normal...
				
				DrawImage player, px, py, 0, 0.25, 0.25
				
			Case 2

				RotateDisplay px, py, ang
				
				SetColor 0, 64, 0
				DrawRect -DeviceWidth, DeviceHeight * 0.5, DeviceWidth * 4.0, DeviceHeight * 3.0 ' Just making sure...
					
				For Local b:Block = Eachin blocklist
					DrawImage b.image, b.x, b.y, 0, 0.25, 0.25
				Next
		
				' Display still set to rotated state, so player movements are affected by rotation,
				' ie. relative to background...
				
				DrawImage player, px, py, 0, 0.25, 0.25

				ResetDisplay
				
			Case 3

				RotateDisplay DeviceWidth * 0.5, DeviceHeight * 0.5, ang
				
				SetColor 0, 64, 0
				DrawRect -DeviceWidth, DeviceHeight * 0.5, DeviceWidth * 4.0, DeviceHeight * 3.0 ' Just making sure...
					
				For Local b:Block = Eachin blocklist
					DrawImage b.image, b.x, b.y, 0, 0.25, 0.25
				Next
		
				ResetDisplay
				
				DrawImage player, px, py, 0, 0.25, 0.25
				
			Case 4

				RotateDisplay DeviceWidth * 0.5, DeviceHeight * 0.5, ang
				
				SetColor 0, 64, 0
				DrawRect -DeviceWidth, DeviceHeight * 0.5, DeviceWidth * 4.0, DeviceHeight * 3.0 ' Just making sure...
					
				For Local b:Block = Eachin blocklist
					DrawImage b.image, b.x, b.y, 0, 0.25, 0.25
				Next
		
				DrawImage player, px, py, 0, 0.25, 0.25
				
				ResetDisplay
				
		End
		
		SetColor 255, 255, 255
		DrawText "[CURSORS - move player] [SPACE - stop player]", 20, 20
		DrawText "[Z/X - rotate around player] [ENTER - stop rotation]", 20, 40

		Local mode$ = ""
		
		Select rotatemode
			Case 1
				mode = "Rotate around player (controls relative to screen)"
			Case 2
				mode = "Rotate with player (controls relative to background)"
			Case 3
				mode = "Rotate around centre (controls relative to screen)"
			Case 4
				mode = "Rotate around centre (controls relative to background)"
		End
		
		DrawText "[R - change rotation mode: " + mode, 20, 60
		
	End
	
End

' Main function...

Function Main ()
	New Game
End

' Set image's handle to centre...

Function MidHandle (image:Image)
		 image.SetHandle image.Width () * 0.5, image.Height () * 0.5
End

' Load an image with handle already centered (uses MidHandle function above)...

Function LoadCenteredImage:Image (image:String)
		 Local img:Image = LoadImage (image)
		 MidHandle img
		 Return img
End
