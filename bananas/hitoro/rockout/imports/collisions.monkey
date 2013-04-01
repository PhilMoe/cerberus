
Import rockout

Function Distance:Float (x1:Float, y1:Float, x2:Float, y2:Float)
	Return Sqrt ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))
End

Function CirclesCollide (go1:Sprite, go2:Sprite, go1jimmy:Float = 0.425, go2jimmy:Float = 0.425)

	Local radius1:Float = go1.width * go1jimmy ' HACK! Scale radius down a bit for non-circular shapes, ie. most of them...
	Local radius2:Float = go2.width * go2jimmy ' DOUBLE-HACK!
	
	Local dist:Float = Distance (go1.x, go1.y, go2.x, go2.y)
	
	If dist < radius1 + radius2
		Return True
	Endif
	
End

Function PointInRect:Int (x:Float, y:Float, rx1:Float, ry1:Float, rx2:Float, ry2:Float)
	If x >= rx1 And x <= rx2 And y >= ry1 And y <= ry2 Then Return True
End Function

Function LinesIntersect:Float (ax:Float, ay:Float, bx:Float, by:Float, cx:Float, cy:Float, dx:Float, dy:Float)

	Local lambda:Float
	Local mu:Float
	
	bx = bx - ax
	by = by - ay
	dx = dx - cx
	dy = dy - cy
	
	If dx <> 0
		lambda = (cy - ay + (ax - cx) * dy / dx) / (by - bx * dy / dx)
	Else
		lambda = (cx - ax + (ay - cy) * dx / dy) / (bx - by * dx / dy)
	EndIf
	
	If bx <> 0
		mu = (ay - cy + (cx - ax) * by / bx) / (dy - dx * by / bx)
	Else
		mu = (ax - cx + (cy - ay) * bx / by) / (dx - dy * bx / by)
	EndIf
	
	If lambda >= 0 And lambda <= 1
		If mu >= 0 And mu <= 1 Then Return lambda
	EndIf
	
	Return -1
	
End Function

' Block collisions...

Const BOX_LEFT:Int		= 1
Const BOX_RIGHT:Int		= 2
Const BOX_TOP:Int		= 4
Const BOX_BOTTOM:Int	= 8

' RectHit returns an integer containing above flags if point x, y is inside rectangle...

Function RectHit:Int (x:Float, y:Float, x1:Float, y1:Float, x2:Float, y2:Float, lastx:Float, lasty:Float)

	Local cmask:Int = 0
	
	If PointInRect (x, y, x1, y1, x2, y2)

		' Check last known position before collision, draw line from
		' x and y to outside position, check if it crosses any of the
		' box's edges...
		
		If lastx <= x1 ' Outside left
			If LinesIntersect (x, y, lastx, lasty, x1, y1, x1, y2) <> -1
				cmask = cmask | BOX_LEFT
			EndIf
		Else
			If lastx >= x2 ' Outside right
				If LinesIntersect (x, y, lastx, lasty, x2, y1, x2, y2) <> -1
					cmask = cmask | BOX_RIGHT
				EndIf
			EndIf
		EndIf

		If lasty <= y1 ' Outside top
			If LinesIntersect (x, y, lastx, lasty, x1, y1, x2, y1) <> -1
				cmask = cmask | BOX_TOP
			EndIf
		Else
			If lasty >= y2 ' Outside bottom
				If LinesIntersect (x, y, lastx, lasty, x1, y2, x2, y2) <> -1
					cmask = cmask | BOX_BOTTOM
				EndIf
			EndIf
		EndIf
	
	EndIf
	
	Return cmask
	
End

Function CheckCollisions ()

	' PLAYER
	' List of BLOCKS
	' List of SHOTS
	
	' PLAYER: Hits BLOCKS
	' SHOTS: Hit PLAYER and BLOCKS which aren't falling

	' ----------------------------------------------------------
	' SHOTS against BLOCKS and PLAYER...
	' ----------------------------------------------------------

	' Blocks on screen? Check blocks against player; also check
	' against shots, so while we're iterating through the shots,
	' might as well check those against the player too. (Saves
	' two runs through shot list, sticking it in this Else block.)
		
	For Local s:Shot = Eachin GameSession.CurrentLevel.Shots

		' --------------------------------------------------
		' SHOTS to BLOCKS...
		' --------------------------------------------------

		' Brutally hacked in from BlitzMax version!
		
		s.lastx = s.x - FrameScale (s.xs)
		s.lasty = s.y - FrameScale (s.ys)

		' Offset x/y position of shots (all images' handles are centered)...
		
		Local ox:Float = s.x - s.width * 0.5
		Local oy:Float = s.y - s.height * 0.5
			
		For Local b:Block = Eachin GameSession.CurrentLevel.Blocks

			' Get x offset of block (mid-handled)...

			Local bx:Int = b.x - b.width * 0.5
			Local by:Int = b.y - b.height * 0.5

			' Check shot collisions with blocks (this only needs rectangular checks!)...

			Local collided:Int = RectHit (s.x, s.y, bx, by, bx + b.width, by + b.height, s.lastx, s.lasty)

			If collided
			
				s.hits = s.hits + 1
				
				' Set block hit time. Used so multiple shots hitting same block
				' within 100 ms get bounced back. Looks more natural than following
				' shots going through...
			
				' Stick score in here since hittime is only set on first hit...

				If b.strength ' 0 = INDESTRUCTIBLE

					b.hitcount = b.hitcount + 1

					If b.hitcount >= b.strength
					
						b.hittime = Millisecs ()

						b.hitby = s ' Block stores which shot hit it -- used for block hit speed

						' Store position when hit...
						
						b.hitx = b.x
						b.hity = b.y
						
						Local score:Int = s.hits * 100
						
						New ScoreBubble b.x, b.y, score
						GameSession.Score = GameSession.Score + score

						b.Fall
						
					EndIf

				EndIf

'							ExplosionParticle.Explode bx, by, 4

				If collided & 1 ' Left
					s.xs = -s.xs
					s.x = s.x + FrameScale (s.xs)
				Else
					If collided & 2 ' Right
						s.xs = -s.xs
						s.x = s.x + FrameScale (s.xs)
					EndIf
				EndIf
			
				If collided & 4 ' Top
					If s.ys > 0.25 Then s.ys = -s.ys * 0.75 Else b.Delete ' For "indestructable" blocks
					s.y = s.y + FrameScale (s.ys)
				Else
					If collided & 8 ' Bottom
						s.ys = -s.ys
						s.y = s.y + FrameScale (s.ys)
					EndIf
				EndIf

			EndIf

		Next

		' Hit falling blocks if first hit was less than 'x' ms ago. All compared against
		' block's original location when hit, giving illusion of multiple shots being able
		' to hit it without having to deal with moving collisions!
		
		For Local b:Block = Eachin GameSession.CurrentLevel.FallingBlocks

			' If block was hit by another shot within 'x' ms of first hit...
			
			If Millisecs < b.hittime + 120

				' Get x offset of block WHEN IT WAS HIT (mid-handled)...
	
				Local bx:Int = b.hitx - b.width * 0.5
				Local by:Int = b.hity - b.height * 0.5
	
				' Check shot collisions with blocks (this only needs rectangular checks!)...
	
				Local collided:Int = RectHit (s.x, s.y, bx, by, bx + b.width, by + b.height, s.lastx, s.lasty)
	
				If collided
				
	'							ExplosionParticle.Explode bx, by, 4
	
					If collided & 1 ' Left
						s.xs = -s.xs
						s.x = s.x + FrameScale (s.xs)
					Else
						If collided & 2 ' Right
							s.xs = -s.xs
							s.x = s.x + FrameScale (s.xs)
						EndIf
					EndIf
				
					If collided & 4 ' Top
						If s.ys > 0.25 Then s.ys = -s.ys * 0.75 Else b.Delete ' For "indestructable" blocks
						s.y = s.y + FrameScale (s.ys)
					Else
						If collided & 8 ' Bottom
							s.ys = -s.ys
							s.y = s.y + FrameScale (s.ys)
						EndIf
					EndIf
	
				EndIf

			Endif
			
		Next
	
		' --------------------------------------------------
		' SHOTS to PLAYER...
		' --------------------------------------------------

		If Not s.inert
			If CirclesCollide (s, GameSession.Player)
				GameSession.Player.Damage s.player_damage
				s.Delete
			Endif
		Endif
		
	Next

	' ------------------------------------------------------
	' PLAYER to BLOCK...
	' ------------------------------------------------------

	For Local b:Block = Eachin GameSession.CurrentLevel.Blocks
		If CirclesCollide (b, GameSession.Player)
			GameSession.Player.Damage 4 ' Test!
			b.Delete
		Endif
	Next

	For Local b:Block = Eachin GameSession.CurrentLevel.FallingBlocks
		If CirclesCollide (b, GameSession.Player)
			GameSession.Player.Damage 4 ' Test!
			b.Delete
		Endif
	Next

End
