
' -----------------------------------------------------------------------------
' Rotate background around player...
' -----------------------------------------------------------------------------

' Demo 2. Quick hack showing rotation while jumping. Worst jumping code ever,
' hard-coded rotation speed for jump height, and very buggy... but that's not
' the point! It rotates correctly around the player, that's all.

' Move using LEFT/RIGHT and hit SPACE to jump really badly...



' -----------------------------------------------------------------------------
' Basic imports for most purposes...
' -----------------------------------------------------------------------------

Import mojo

' -----------------------------------------------------------------------------
' DISPLAY ROTATION FUNCTIONS...
' -----------------------------------------------------------------------------



' -----------------------------------------------------------------------------
' RotateDisplay: rotates view around x, y by angle...
' -----------------------------------------------------------------------------

	Function RotateDisplay (X:Float, Y:Float, angle:Float)
		PushMatrix				' Store current rotation, scale, etc
		Translate X, Y			' Shift origin across to here
		Rotate angle			' Rotate around origin
		Translate -X, -Y		' Shift origin back where it was
	End

' -----------------------------------------------------------------------------
' ResetDisplay: resets view back to normal...
' -----------------------------------------------------------------------------

	Function ResetDisplay ()
		PopMatrix				' Restore rotation, scale, etc
	End



' -----------------------------------------------------------------------------
' USAGE...
' -----------------------------------------------------------------------------

' Step 1) In OnRender, call RotateDisplay with x/y position to rotate around
' and the angle at which to draw...

' Step 2) DRAW BACKGROUND STUFF HERE!

' Step 3) Call ResetDisplay and continue drawing as normal...

' See OnRender code for example...



' -----------------------------------------------------------------------------
' TOP TIP! You can use this call to just rotate around screen centre...
' -----------------------------------------------------------------------------

' 	RotateDisplay DeviceWidth * 0.5, DeviceHeight * 0.5, ang




Class Block
	Field X:Float
	Field Y:Float
	Field Image:Image
End

' App class...

Class Game Extends App

	Field gravity:Float = 0.05
	
	Field player:Image
	Field block:Image
	
	Field px:Float
	Field py:Float

	Field pxs:Float
	Field pys:Float

	Field groundy:Float
	
	Field ang:Float
	Field angspeed:Float

	Field rotating:Bool
	
	Field blocklist:List <Block>
	
	Method OnCreate ()		' Load media and SET UPDATE RATE here...

'		px = DeviceWidth * 0.5
		py = DeviceHeight * 0.5	

		player	= LoadCenteredImage ("player.png")
		block	= LoadCenteredImage ("block.png")
		
		blocklist = New List <Block>
		
		For Local loop:Int = 1 To 100
			Local temp:Block = New Block
			temp.Image = block
			temp.X = Rnd (-DeviceWidth, DeviceWidth)
			temp.Y = Rnd (-DeviceHeight, DeviceHeight)
			blocklist.AddLast temp
		Next
		
		groundy = 300 + (player.Height * 0.5) * 0.25
		
		SetUpdateRate 60
		
	End
	
	Method OnUpdate ()		' Take input and update game here...

		Local movinglr:Bool
		
		If Not rotating

			If KeyDown (KEY_LEFT)
				pxs = pxs - 0.1
				movinglr = True
			Endif
			
			If KeyDown (KEY_RIGHT)
				pxs = pxs + 0.1
				movinglr = True
			Endif
		
			If py >= 295
				If KeyDown (KEY_SPACE)
					pys = -3
					rotating = True
				Endif
			Endif

		Endif
		
		If pxs > 2 Then pxs = 2
		If pxs < -2 Then pxs = -2
		px = px + pxs

		If Not movinglr And py >= 295 Then pxs = pxs * 0.99
		
		pys = pys + gravity
		py = py + pys
		If py > 300
			py = 300
			pys = -pys * 0.5
		Endif
		
		If px < 0 Or px > DeviceWidth
			pxs = -pxs
			px = px + pxs
		Endif

		If py < 0 Or py > DeviceHeight
			pys = -pys
			py = py + pys
		Endif
		
		If rotating
			ang = ang + 3 * Sgn (pxs)
			If ang < -360 Or ang > 360
				ang = 0
				rotating = False
			Endif
		Endif

	End
	
	Method OnRender ()		' Draw frame here...
	
		Cls 64, 96, 128

		' ---------------------------------------------------------------------
		' Rotate display around player position...
		' ---------------------------------------------------------------------
			RotateDisplay px, py, ang
		' ---------------------------------------------------------------------

		' DRAW BACKGROUND...
				
		SetColor 0, 64, 0
		DrawRect -DeviceWidth, groundy, DeviceWidth * 4.0, DeviceHeight * 3.0 ' Just making sure...
	
		SetColor 255,255,255
		For Local B:Block = Eachin blocklist
			DrawImage B.Image, B.X, B.Y, 0, 0.25, 0.25
		Next
	
		DrawText "-11 -10 -09 -08 -07 -06 -05 -04 -03 -02 -01 000 +01 +02 +03 +04 +05 +06 +07 +08 +09 +10 + 11", 0, groundy
		
		' ---------------------------------------------------------------------
		' Set display back to previous (ie. normal) state...
		' ---------------------------------------------------------------------
			ResetDisplay
		' ---------------------------------------------------------------------
		
		' DRAW PLAYER AT NORMAL ORIENTATION and DRAW DISPLAY TEXT...
		
		DrawImage player, px, py, 0, 0.25, 0.25
		DrawText "[CURSORS - move player] [SPACE - jump]", 20, 20

	End
	
End

' Main function...

Function Main ()
	New Game
End

' Set image's handle to centre...

Function MidHandle (Image:Image)
		 Image.SetHandle Image.Width () * 0.5, Image.Height () * 0.5
End

' Load an image with handle already centered (uses MidHandle function above)...

Function LoadCenteredImage:Image (Image:String)
		 Local img:Image = LoadImage (Image)
		 MidHandle img
		 Return img
End
