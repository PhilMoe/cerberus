
Import rockout

Class Shot Extends Sprite

	Field parent:List <Shot>
	
	Field hits:Int ' Number of blocks hit (score increases with each hit)
	
	Field lastx:Float
	Field lasty:Float
	
	Field inert:Int = True ' Shots start off inert, until they start to fall. Prevents player colliding while shooting...
	
	Method Delete ()
		parent.RemoveEach Self
	End
	
	Global ReloadDelay:Int
	Global FirstShot:Int

	Field player_damage:Float = 2.0
	
	Method New (img:Image, x:Float, y:Float, xs:Float, ys:Float, xscale:Float, yscale:Float)

		' ---------------------------------------------------------------------
		' First shot delay logic...
		' ---------------------------------------------------------------------

		If Shot.FirstShot
		
			' If this is first shot, next shot only happens after 160 ms, to allow for single-shot tap...
			
			Shot.ReloadDelay = 160
			Shot.FirstShot = False
			
		Else
		
			' If not first shot, rapid-fire...
			
			Shot.ReloadDelay = 80
			
		Endif

		Self.image = img

		Self.x = x
		Self.y = y
		
		Self.xs = xs * 0.2
		Self.ys = -4.0

		Self.xscale = xscale
		Self.yscale = yscale

		' This is the pixel width of the image after scaling...
		
		Self.width = img.Width * xscale
		Self.height = img.Height * yscale

		GameSession.CurrentLevel.Shots.AddLast Self
	
		Self.parent = GameSession.CurrentLevel.Shots
		
	End
	
	Function UpdateAll ()
	
		If GameSession.CurrentLevel.Shots = Null Then Return
		
		' Pre-calc for loop...
		
		Local vdw:Float = VDeviceWidth ()
		Local vdh:Float = VDeviceHeight ()
		
		For Local s:Shot = Eachin GameSession.CurrentLevel.Shots
		
			s.ys = s.ys + FrameScale (GameSession.CurrentLevel.Gravity)
			
			If s.inert
				If s.ys > 0
					s.inert = False
				Endif
			Endif
			
			s.x = s.x + FrameScale (s.xs)
			s.y = s.y + FrameScale (s.ys)
			
			If s.x < 0 Or s.x > vdw
				s.xs = -s.xs
				s.x = s.x + FrameScale (s.xs)
			Endif

			If s.y > vdh + 64
				GameSession.CurrentLevel.Shots.RemoveEach s
			Endif
					
		Next
	
	End

	Function Render ()
		For Local s:Shot = Eachin GameSession.CurrentLevel.Shots
			s.Draw
		Next
	End

End
