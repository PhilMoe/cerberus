

Import mojo.app
Import mojo.graphics
Import mojo.input

' TO DO -- Limit mouse x/y to visible (zoomed) area, rather than just virtual
' display area? Only when ratioborders kicks in?

' -----------------------------------------------------------------------------
' Usage...
' -----------------------------------------------------------------------------

' -----------------------------------------------------------------------------
' SetVirtualDisplay (width, height)
' -----------------------------------------------------------------------------

' Call during OnCreate, passing intended width and height of game area. Design
' your game for this fixed display size and it will be scaled correctly on any
' device. You can pass no parameters for default 640 x 480 virtual device size.

' -----------------------------------------------------------------------------
' ScaleDisplay
' -----------------------------------------------------------------------------

' Call at start of OnRender, BEFORE ANYTHING ELSE, including Cls!

' -----------------------------------------------------------------------------
' VMouseX/VMouseY
' -----------------------------------------------------------------------------

' Call during OnUpdate (or OnRender) to get correctly translated MouseX/MouseY
' positions. By default, the results are bound to the display area within the
' borders. You can override this by passing False as an optional parameter,
' and the functions will then return values outside of the borders.

' -----------------------------------------------------------------------------
' VDeviceWidth/VDeviceHeight
' -----------------------------------------------------------------------------

' Call during OnUpdate (or OnRender) for the virtual device width/height. These
' are just the values you passed to SetVirtualDisplay.

' -----------------------------------------------------------------------------
' Functions...
' -----------------------------------------------------------------------------

' -----------------------------------------------------------------------------
' SetVirtualDisplay: Call in OnCreate...
' -----------------------------------------------------------------------------

' Parameters: width and height of virtual game area...

Function SetVirtualDisplay (width:Int = 640, height:Int = 480)
	New VirtualDisplay (width, height)
End

' -----------------------------------------------------------------------------
' ScaleDisplay: Call at start of OnRender...
' -----------------------------------------------------------------------------

' Parameter zoom allows you to zoom in and out within the virtual display;

' The 'zoomborders' parameter can be set to False to allow you to retain fixed
' width/height borders for the current display size/ratio. By default, the
' borders scale with the zoom factor. See VMouseX/Y information for more
' details on how this can be used...

' The 'ratioborders' parameter means the outer aspect ratio borders are kept
' no matter how zoomed in the game is. Setting this to False means you can
' zoom into the game, the borders appearing to go 'outside' the screen as
' you zoom further in. You'll have to try it to get it, but it only affects
' zooming inwards...

Function ScaleDisplay (zoom:Float = 1.0, zoomborders:Bool = True, ratioborders:Bool = True)
	VirtualDisplay.Display.ScaleDisplay zoom, zoomborders, ratioborders
End

' -----------------------------------------------------------------------------
' Misc functions: Call in OnUpdate (optionally)...
' -----------------------------------------------------------------------------

' Mouse position within virtual display; the limit parameter allows you to only
' return values within the virtual display.

' Set the 'limit' parameter to False to allow returning of values outside
' the virtual display area. Combine this with ScaleDisplay's zoomborders
' parameter set to False if you want to be able to zoom way out and allow
' gameplay in the full zoomed-out area... 

Function VMouseX:Float (limit:Bool = True)
	Return VirtualDisplay.Display.VMouseX (limit)
End

Function VMouseY:Float (limit:Bool = True)
	Return VirtualDisplay.Display.VMouseY (limit)
End

' Virtual display size...

Function VDeviceWidth:Float ()
	Return VirtualDisplay.Display.vwidth
End

Function VDeviceHeight:Float ()
	Return VirtualDisplay.Display.vheight
End

' -----------------------------------------------------------------------------
' Virtual Display System... [Public domain code]
' -----------------------------------------------------------------------------

Class VirtualDisplay

	' NOTES
	
		' CORRECT MOUSE FRACTION OF DEVICE WIDTH
	
		'Local cfrac:Float = (MouseX - (Float (DeviceWidth) * 0.5)) / Float (DeviceWidth)
		
		' WIDTH OF SCALED VIRTUAL DISPLAY IN PIXELS
		
		'Local real:Float = (vwidth * zoom * multi)

		' CORRECT SPACE BETWEEN SCALED DEVICE AND DEVICE
		
		' Local offx:Float = (Float (DeviceWidth) - real)' * 0.5

	Global Display:VirtualDisplay
	
	Field vwidth:Float					' Virtual width
	Field vheight:Float					' Virtual height

	Field vratio:Float					' Virtual ratio

	Field scaledw:Float					' Width of *scaled* virtual display in real pixels...
	Field scaledh:Float					' Width of *scaled* virtual display in real pixels...

	Field widthborder:Float				' Size of border at sides
	Field heightborder:Float			' Size of border at top/bottom

	Field multi:Float					' Ratio scale factor
	Field zoom:Float = 1.0				' Zoom scale factor
	
	Method New (width:Int, height:Int)

		' Set virtual width and height...
			
		vwidth = width
		vheight = height

		' Store ratio...
		
		vratio = vheight / vwidth

		' Create global VirtualDisplay object...
		
		Display = Self
	
	End

	Method VMouseX:Float (limit:Bool)
		
		' Position of mouse, in real pixels, from centre of screen (centre being 0)...
		
		Local mouseoffset:Float = MouseX - Float (DeviceWidth) * 0.5
		
		' This calculates the scaled position on the virtual display. Somehow...
		
		Local x:Float = (mouseoffset / multi) / zoom + (VDeviceWidth * 0.5)

		' Check if mouse is to be limited to virtual display area...
		
		If limit
	
			Local widthlimit:Float = vwidth - 1
	
			If x > 0
				If x < widthlimit
					Return x
				Else
					Return widthlimit
				Endif
			Else
				Return 0
			Endif
	
		Else
			Return x
		Endif
	
	End

	Method VMouseY:Float (limit:Bool)
	
		' Position of mouse, in real pixels, from centre of screen (centre being 0)...

		Local mouseoffset:Float = MouseY - Float (DeviceHeight) * 0.5
		
		' This calculates the scaled position on the virtual display. Somehow...

		Local y:Float = (mouseoffset / multi) / zoom + (VDeviceHeight * 0.5)
		
		' Check if mouse is to be limited to virtual display area...

		If limit
		
			Local heightlimit:Float = vheight - 1
		
			If y > 0
				If y < heightlimit
					Return y
				Else
					Return heightlimit
				Endif
			Else
				Return 0
			Endif

		Else
			Return y
		Endif
		
	End

	Method ScaleDisplay (tzoom:Float, zoomborders:Bool, ratioborders:Bool)

		' Store device resolution as float values to avoid loads of casts. Doing it here in
		' case it turns out to be possible to change device resolution on the fly at some point...
		
		Local fdw:Float = Float (DeviceWidth)
		Local fdh:Float = Float (DeviceHeight)
		
		' Device ratio is calculated on the fly since it can change (eg. resizeable
		' browser window)...
		
		Local dratio:Float = fdh / fdw
		
		' Compare to pre-calculated virtual device ratio...
		
		If dratio >= vratio

			' -----------------------------------------------------------------
			' Device aspect narrower than (or same as) game aspect ratio:
			' will use full width, borders above and below...
			' -----------------------------------------------------------------

			' Multiplier required to scale game width to device width (to be applied to height)...
			
			multi = fdw / vwidth
			
			' "vheight * multi" below applies width multiplier to height...
			
			heightborder = (fdh - vheight * multi) * 0.5
			widthborder = 0
			
		Else

			' -----------------------------------------------------------------
			' Device aspect wider than game aspect ratio:
			' will use full height, borders at sides...
			' -----------------------------------------------------------------
			
			' Multiplier required to scale game height to device height (to be applied to width)...
			
			multi = fdh / vheight
			
			' "vwidth * multi" below applies height multiplier to width...

			widthborder = (fdw - vwidth * multi) * 0.5
			heightborder = 0

		Endif
		
		' ---------------------------------------------------------------------
		' Clear outer area (black borders if required)...
		' ---------------------------------------------------------------------
		
		SetScissor 0, 0, DeviceWidth, DeviceHeight
		Cls 0, 0, 0
		
		' ---------------------------------------------------------------------
		' Set inner area...
		' ---------------------------------------------------------------------

		If zoomborders

			' Width/height of SCALED virtual display in real pixels...
			
			Local realx:Float = (vwidth * zoom * multi)
			Local realy:Float = (vheight * zoom * multi)
	
			' Space in pixels between real device borders and virtual device borders...
			
			Local offx:Float = (Float (DeviceWidth) - realx) * 0.5
			Local offy:Float = (Float (DeviceHeight) - realy) * 0.5

'			SetScissor offx, offy, Float (DeviceWidth) - (offx * 2.0), Float (DeviceHeight) - (offy * 2.0)
'			SetScissor widthborder, heightborder, fdw - widthborder * 2.0, fdh - heightborder * 2.0

			' WIP: Retain borders when zoomed in... add option to lose borders...
			' Can ratioborders check go outside?
			
			Local sx:Float, sy:Float, sw:Float, sh:Float

			If ratioborders

				If offx < widthborder
					sx = widthborder
					sw = fdw - widthborder * 2.0
				Else
					sx = offx
					sw = Float (DeviceWidth) - (offx * 2.0)
				Endif

			Else

				sx = offx
				sw = Float (DeviceWidth) - (offx * 2.0)

			Endif
			
			If ratioborders
	
				If offy < heightborder
					sy = heightborder
					sh = fdh - heightborder * 2.0
				Else
					sy = offy
					sh = Float (DeviceHeight) - (offy * 2.0)
				Endif

			Else

				sy = offy
				sh = Float (DeviceHeight) - (offy * 2.0)

			Endif
			
			SetScissor sx, sy, sw, sh
			
		Else
			SetScissor widthborder, heightborder, fdw - widthborder * 2.0, fdh - heightborder * 2.0
		Endif
		
		' ---------------------------------------------------------------------
		' Scale and translate everything...
		' ---------------------------------------------------------------------
		
		zoom = tzoom ' Copy passed zoom factor to local zoom field...
		
		Scale multi * zoom, multi * zoom

		' ---------------------------------------------------------------------
		' Shift display to account for borders/zoom level...
		' ---------------------------------------------------------------------

		If zoom ' Gets skipped if zero...
		
			' Width and height of *scaled* virtual display in pixels...

			scaledw = (vwidth * multi * zoom)
			scaledh = (vheight * multi * zoom)

			' Find offsets by which view needs to be shifted...
			
			Local xoff:Float = (fdw - scaledw) * 0.5
			Local yoff:Float = (fdh - scaledh) * 0.5

			' Ahh, good old trial and error -- I have no idea how this works!
			
			xoff = (xoff / multi) / zoom
			yoff = (yoff / multi) / zoom
			
			' Aaaand, shift...
			
			Translate xoff, yoff
		
		Endif
		
	End

End
