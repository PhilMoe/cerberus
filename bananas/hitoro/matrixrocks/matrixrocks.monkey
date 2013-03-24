
' -----------------------------------------------------------------------------
' Matrix demo...
' -----------------------------------------------------------------------------

' -----------------------------------------------------------------------------
' Just to show some simple tricks you can perform with the mojo matrix!
' -----------------------------------------------------------------------------

' Note that simple scaling and rotation can be applied by calling the longer
' version of DrawImage (see docs), so don't assume you HAVE to use the matrix
' commands if you find it confusing!

' IMPORTANT: For simplicity, it is recommended that you set images' drawing
' handles to their centres. To see why, try changing the LoadCenteredImage
' function call (in OnCreate) to LoadImage. This explanation assumes centered
' image handles!

' -----------------------------------------------------------------------------
' DISPLAY MATRIX:
' -----------------------------------------------------------------------------

' The display matrix stores the settings to be used for all following drawing
' operations, such as scale, rotation and translation (offset).

' -----------------------------------------------------------------------------
' DRAWING ORIGIN:
' -----------------------------------------------------------------------------

' The drawing 'origin' is the point on the screen from which all drawing
' operations are offset. By default it's at the top-left of the screen
' (position 0, 0), so that "DrawImage player, 50, 50" would draw the image at
' 50, 50 as you'd expect.

' Calling "DrawImage player, 0, 0" would draw it at the top-left of the screen,
' but if you were to first move the origin to 50, 50 by calling "Translate 50,
' 50", the same "DrawImage player, 0, 0" command would draw the player at 50,
' 50; the origin is now at 50, 50 and the DrawImage command is offsetting the
' image 0, 0 from that point.

' Any other commands, such as Scale and Rotate will also occur from the current
' origin; for example, scaling up causes all drawing operations to increase in
' size 'outwards' from the current origin. Rotation will take effect around the
' current origin.

' Therefore, in most cases, you need to FIRST move the origin to where you want
' a particular effect to take place, THEN scale or rotate as required, usually
' calling DrawImage with x and y values of 0. Any other x or y values will
' cause the drawing to take effect offset by those values from the current
' origin.

' -----------------------------------------------------------------------------
' TRANSLATE:
' -----------------------------------------------------------------------------

' Translate is similar to SetOrigin in other languages, eg. BlitzMax, and
' simply sets the drawing origin for all following commands. Rotation and
' scaling will take effect around this origin, and all drawing commands' x/y
' values will be offset from this origin. (They normally are anyway, it's just
' that the origin is at 0, 0 by default.)

' -----------------------------------------------------------------------------
' SCALE:
' -----------------------------------------------------------------------------

' Everything is scaled up or down, with the origin as central scaling
' point. Eg. using the default origin of 0, 0, scaling everything up by 2.0
' will cause all drawing to scale outwards from the top-left of the screen, and
' all drawing will appear to be scaled out and towards the bottom-right.

' In fact, it scales outwards in all directions around the origin; set the
' origin to the centre of the screen and the effect will be that everything
' scales outwards in all directions -- a 'camera zooming' effect.

' All drawing positions will be scaled by this amount too, for example,
' "DrawImage player, 10, 10" with scale set to 2.0 will have the same effect
' as calling' "DrawImage player, 20, 20" with the default scale of 1.0.

' It helps to think of DrawImage's x and y values (along with DrawRect, etc)
' are offsets from the current origin -- that's what they really are.

' -----------------------------------------------------------------------------
' ROTATE:
' -----------------------------------------------------------------------------

' All following drawing operations will be rotated by this amount, around
' the current origin...

' -----------------------------------------------------------------------------
' COMBINED OPERATIONS:
' -----------------------------------------------------------------------------

' This is where things can get confusing really quickly! All matrix operations
' have a knock-on effect on each other, and the order in which these operations
' are performed will give different visual results.

' Assume the origin (set by Translate) is at its default position of 0, 0, the
' top-left of the screen. Calling "Rotate 45" means that any image will be
' draw rotated by 45 degrees, but its drawing offsets will also be offset
' by 45 degrees, so "DrawImage player, 50, 0" won't draw the image at 50 pixels
' across the screen, but 50 pixels in the 45-degree direction!

' If, however, you call "Translate 50, 0" (putting the origin for scale,
' rotation, and so on at position 50, 0), calling "Rotate 45" will still
' apply rotation, but if you then call "DrawImage player, 0, 0" -- zero offset
' from the current origin of 50, 0 -- the rocket will be in the right place
' and rotated by 45 degrees.

' -----------------------------------------------------------------------------
' SAVING/RESTORING MATRIX STATES:
' -----------------------------------------------------------------------------

' The display starts with its matrix reset to default at the start of every
' frame, ie, origin at 0, 0, no rotation and scale set to 1.0.

' Let's say we want to draw a background rotated by 45 degrees but draw the
' player at 'normal' orientation; to do this:

' 1) We need to save the current 'default' matrix state, so we can put it back
' as it was after playing with it, so call PushMatrix. (Think of this as SAVING
' the current state of the matrix.)

' 2) Apply rotation to the background. To rotate around the centre of the
' screen, we need to move the drawing origin to the centre (half of DeviceWidth
' and half of DeviceHeight), rotate around this point (Rotate 45), then draw
' the background. (If it's a single image, remember to set its handle to the
' centre.)

' 3) To put the matrix back how it was, call PopMatrix, which can be thought
' of as RESTORING the previously saved matrix state.

' 4) Drawing will now take place with the previously saved matrix settings, so
' "DrawImage player, x, y" will behave as normal.

' -----------------------------------------------------------------------------
' NESTING MATRIX STATES:
' -----------------------------------------------------------------------------

' You can do more complex saving and restoring of the matrix by, for example,
' saving a rotated state (where you might have drawn a background at 45
' degrees), applying further rotation, scaling, etc, to draw other objects,
' then calling PopMatrix to restore the saved rotated state to draw more
' objects with the same settings as the background.

' Each pair of PushMatrix/PopMatrix calls saves and restores the previously
' saved state. Assuming the matrix hasn't been changed from its reset state
' at the start of OnRender, this example goes through the matrix states and
' how they are saved and restored:

' -----------------------------------------------------------------------------
'
'	' MATRIX STATE #1 (default matrix at start of OnRender)
'	
'	PushMatrix						' Save state #1
'	
'		' MATRIX STATE #2:
'		
'		Scale 2, 2
'		DrawImage player, 0, 0
'	
'		PushMatrix					' Save state #2
'	
'			' MATRIX STATE #3:
'			
'			Translate 20, 20
'			DrawImage player, 0, 0
'	
'			PushMatrix				' Save state #3
'	
'				' MATRIX STATE #4:
'
'				Rotate 180
'				DrawImage player, 0, 0
'	
'			PopMatrix				' Restore state #3
'	
'		PopMatrix					' Restore state #2
'	
'	PopMatrix						' Restore state #1
'
'	' BACK TO MATRIX STATE #1, THE DEFAULT MATRIX STATE

' -----------------------------------------------------------------------------

' I would recommend, for each matrix change, writing this first of all:

' 	PushMatrix
' 	PopMatrix

' ... and THEN going back and filling in the drawing commands, indenting each pair
' as you would with For/Next, If/Endif, etc:

' 	PushMatrix
'
'		' Translate/rotate/scale/draw here...
'
' 		PushMatrix
'
'			' Translate/rotate/scale/draw here...
'
' 		PopMatrix
'
' 	PopMatrix

' This nesting of matrix states is what the example below demonstrates. See
' the OnRender code for details and further explanation...

' Experiment in small steps, eg. save the default state, make some changes and
' draw with those changes, restore the default state and draw again. Once you
' see how it works, add another PushMatrix/PopMatrix pair and make further
' changes, noting the effect on drawing in that state.

' If it all ultimately makes no sense and you just want to draw scaled,
' rotated images as easily as possible, read the documentation for DrawImage
' and you'll be fine!

' -----------------------------------------------------------------------------
' Basic imports for most purposes...
' -----------------------------------------------------------------------------

Import mojo

' Misc functions...

Function MidHandle (image:Image)
	image.SetHandle image.Width () * 0.5, image.Height () * 0.5
End

Function LoadCenteredImage:Image (image:String)
	Local img:Image = LoadImage (image)
	MidHandle img
	Return img
End

' App class...

Class Game Extends App

	Field player:Image
	
	Field mx:Float, my:Float
	
	Field ang:Float		= 0.0
	
	Field alpha:Float		= 0.0
	Field alphadir:Float	= 0.005
	
	Method OnCreate ()
	
		player = LoadCenteredImage ("default_player.png")
		
		SetUpdateRate 60
		
	End
	
	Method OnUpdate ()
	
		mx = MouseX ()
		my = MouseY ()
		
	End
	
	Method OnRender ()
	
		' Just alpha blending stuff...
		
		alpha = alpha + alphadir
		
		If alpha < 0.0 Or alpha > 1.0
			alphadir = -alphadir
			alpha = alpha + alphadir
		Endif
		
		' Update angle...
		
		ang = ang + 2.0
			
		Cls 32, 64, 128
		
		' Store current matrix (the default scale, translation and rotation)...
		
		PushMatrix
		
			' Move draw origin to centre of screen, scale according to alpha
			' (translucency) level and set rotation...
			
			Translate DeviceWidth * 0.5, DeviceHeight * 0.5
			Scale 1.5 * alpha, 1.5 * alpha
			Rotate ang

			' Draw using these settings...
			
			SetAlpha alpha
			DrawImage player, 0, 0

			' Going to apply further scale, rotation, etc, to these settings, so store
			' current settings...
			
			PushMatrix
			
				' Offset 128 (scaled) pixels to left of big rocket, scale down, rotate
				' ang degrees from current rotation...
				
				Translate -128, 0
				Scale 0.5, 0.5
				Rotate ang
				
				DrawImage player, 0, 0
				
				' Put matrix back how it was...
				
			PopMatrix
			
			' Same again. Now back at big rocket's rotation, scale, etc...
			
			PushMatrix
			
				' Offset to right this time, and subtract ang degrees from
				' current rotation...
				
				Translate 128, 0
				Scale 0.5, 0.5
				Rotate -ang
				
				DrawImage player, 0, 0
				
			PopMatrix

			' Back at big rocket's rotation, scale, etc, again...
			
		PopMatrix
		
		' Now back at original (default) matrix settings. Going to make changes, so
		' store matrix...
		
		PushMatrix
		
			' Offset to centre of screen and scale according to ang...
			
			Translate DeviceWidth * 0.5, DeviceHeight * 0.5
			Scale Sin (ang), Cos (ang)

			SetAlpha 1.0

			DrawImage player, 0, 0
		
		PopMatrix
		
		' Matrix back to default. Shift drawing origin to mouse position, scale
		' and draw...
		
		Translate mx, my
		Scale 0.25, 0.25
		DrawImage player, 0, 0
		
	End
	
End

' Main function...

Function Main ()
	New Game
End
