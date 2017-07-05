
Import rockout

Class Rocket Extends Sprite

	Field mousediv:Float = 0.083333			' Used in UpdatePlayer -- movement towards mouse each frame
	Field firepoint:Float					' y-offset for shots, from rocket centre
	Field inplay:Int = False				' "On-screen?" hack to prevent firing at startup
	Field mass:Float = 4.0					' Only used in applying gravity during death...
		
	Private
		Field shields:Float = 100			' Only Rocket methods can modify shields...
	Public
	
	Method New (img:Image, x:Float, y:Float, xscale:Float, yscale:Float)
	
		Self.image = img

		Self.x = x
		Self.y = y
		
		Self.xscale = xscale
		Self.yscale = yscale

		' This is the pixel width of the image after scaling...
		
		Self.width = img.Width * xscale
		Self.height = img.Height * yscale

		' Offset shot fire point by half of player's height...
		
		Self.firepoint = (Self.height * 0.5) + 8.0

	End
	
	Method Shields ()
		If shields < 0 Then shields = 0
		Return shields ' Converted to Int by function type...
	End
		
	Method Damage (damage:Float)
		shields = shields - damage
	End

	Method Alive ()
		If shields > 0
			Return True
		Endif
	End
	
	Method UpdatePlayer (state:Int, shoot:Int = 0)
	
		Select state

			Case STATE_PLAYING
					
				' Distance to cursor...
				
				Local xdist:Float = RockOut.mx - x
				Local ydist:Float = RockOut.my - y
				
				' Calculate 'mousediv' fraction of the distance each frame...
				
				xs = xdist * mousediv
				ys = ydist * mousediv

				' Move player towards cursor by this fraction...
				
				x = x + FrameScale (xs)
				y = y + FrameScale (ys)
				
				' Hack to prevent firing until properly on screen...
				
				If Not inplay
					If y < VDeviceHeight - (height * 0.5) Then inplay = True
				Else
				
					' Fire!
					
					If shoot
			
						' This stuff allows single shots for quick taps, rapid-fire if held...
						
						If Timer.ShotReload.TimeOut (Shot.ReloadDelay)
							Fire
						Endif
			
					Else
					
						' KeyDown timed out, so 'next' first shot takes place right away...
						
						Shot.FirstShot = True
						Shot.ReloadDelay = 0
			
					Endif

				Endif
					
			Case STATE_GAMEOVER
			
				' Get off screen...
				
				x = x + FrameScale (xs)
				
				If x < 0 Or x > VDeviceWidth
					xs = -xs
					x = x + FrameScale (xs)
				Endif
				
				ys = ys + FrameScale (GameSession.CurrentLevel.Gravity) * mass
				y = y + FrameScale (ys)
				
'			Default
'				Print "Unknown state in UpdatePlayer!"

		End
			
	End
	
	Method Fire (img:Image = DEFAULT_SHOT)
	
		Timer.ShotReload.Reset
		
		New Shot (img, x, y - firepoint, xs, ys, 1.0, 1.0)
		
	End

	Method Render ()
		Draw
	End
	
End
