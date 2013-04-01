
Import mojo

' TODO -- Limit mouse x/y to visible (zoomed) area, rather than just virtual
' display area? Only when keepborders kicks in?





' -----------------------------------------------------------------------------
' Usage. For details see function definitions...
' -----------------------------------------------------------------------------





' -----------------------------------------------------------------------------
' SetVirtualDisplay
' -----------------------------------------------------------------------------

' Call during OnCreate, passing intended width and height of game area. Design
' your game for this fixed display size and it will be scaled correctly on any
' device. You can pass no parameters for default 640 x 480 virtual device size.

' Optional zoom parameter default to 1.0.

' -----------------------------------------------------------------------------
' UpdateVirtualDisplay
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
' SetVirtualZoom
' -----------------------------------------------------------------------------

' Call in OnUpdate to set zoom level.

' -----------------------------------------------------------------------------
' AdjustVirtualZoom
' -----------------------------------------------------------------------------

' Call in OnUpdate to zoom in/out by given amount.

' -----------------------------------------------------------------------------
' GetVirtualZoom
' -----------------------------------------------------------------------------

' Call in OnUpdate or OnRender to retrieve current zoom level.






' -----------------------------------------------------------------------------
' Function definitions and parameters...
' -----------------------------------------------------------------------------





' -----------------------------------------------------------------------------
' SetVirtualDisplay: Call in OnCreate...
' -----------------------------------------------------------------------------

' Parameters: width and height of virtual game area, optional zoom...

Function SetVirtualDisplay (width:Int = 640, height:Int = 480, zoom:Float = 1.0)
	New VirtualDisplay (width, height, zoom)
End

' -----------------------------------------------------------------------------
' SetVirtualZoom: Call in OnUpdate...
' -----------------------------------------------------------------------------

' Parameters: zoom level (1.0 being normal)...

Function SetVirtualZoom (zoom:Float)
	VirtualDisplay.Display.SetZoom zoom
End

' -----------------------------------------------------------------------------
' AdjustVirtualZoom: Call in OnUpdate...
' -----------------------------------------------------------------------------

' Parameters: amount by which to change current zoom level. Positive values
' zoom in, negative values zoom out...

Function AdjustVirtualZoom (amount:Float)
	VirtualDisplay.Display.AdjustZoom amount
End

' -----------------------------------------------------------------------------
' GetVirtualZoom: Call in OnUpdate or OnRender...
' -----------------------------------------------------------------------------

' Parameters: none...

Function GetVirtualZoom:Float ()
	Return VirtualDisplay.Display.GetZoom ()
End

' -----------------------------------------------------------------------------
' UpdateVirtualDisplay: Call at start of OnRender...
' -----------------------------------------------------------------------------

' Parameters:

' Gah! Struggling to explain this! Just experiment!

' The 'zoomborders' parameter can be set to False to allow you to retain FIXED
' width/height borders for the current device size/ratio. Effectively, this
' means that as you zoom out, you can see more of the 'playfield' outside the
' virtual display, instead of having borders drawn to fill the outside area.
' See VMouseX/Y information for more details on how this can be used...

' The 'keepborders' parameter, if set to True, means the outer borders are
' kept no matter how ZOOMED IN the game is. Setting this to False means you
' can zoom into the game, the borders appearing to go 'outside' the screen
' as you zoom further in. You'll have to try it to get it, but it only
' affects zooming inwards. NB. ONLY TAKES EFFECT IF zoomborders IS TRUE!

Function UpdateVirtualDisplay (zoomborders:Bool = True, keepborders:Bool = True)
	VirtualDisplay.Display.UpdateVirtualDisplay zoomborders, keepborders
End

' -----------------------------------------------------------------------------
' Misc functions: Call in OnUpdate (optionally)...
' -----------------------------------------------------------------------------

' Mouse position within virtual display; the limit parameter allows you to only
' return values within the virtual display.

' Set the 'limit' parameter to False to allow returning of values outside
' the virtual display area. Combine this with ScaleVirtualDisplay's zoomborders
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
' AutoFit - Virtual Display System... [Public domain code]
' -----------------------------------------------------------------------------

' Couldn't think of a better name!

Class VirtualDisplay

	' MISC NOTES
	
		' CORRECT MOUSE FRACTION OF DEVICE WIDTH
	
		'Local cfrac:Float = (MouseX - (Float (DeviceWidth) * 0.5)) / Float (DeviceWidth)
		
		' WIDTH OF SCALED VIRTUAL DISPLAY IN PIXELS
		
		'Local real:Float = (vwidth * zoom * multi)

		' CORRECT SPACE BETWEEN SCALED DEVICE AND DEVICE
		
		' Local offx:Float = (Float (DeviceWidth) - real)' * 0.5

	Global Display:VirtualDisplay
	
	Private
	
	Field vwidth:Float					' Virtual width
	Field vheight:Float					' Virtual height

	Field vratio:Float					' Virtual ratio

	Field scaledw:Float					' Width of *scaled* virtual display in real pixels
	Field scaledh:Float					' Width of *scaled* virtual display in real pixels

	Field widthborder:Float				' Size of border at sides
	Field heightborder:Float				' Size of border at top/bottom

	Field multi:Float						' Ratio scale factor
	Field vzoom:Float						' Zoom scale factor
	
	Field fdw:Float						' DeviceWidth gets pre-cast to Float in UpdateVirtualDisplay
	Field fdh:Float						' DeviceHeight gets pre-cast to Float in UpdateVirtualDisplay
	
	Public
	
	Method New (width:Int, height:Int, zoom:Float)

		' Set virtual width and height...
			
		vwidth = width
		vheight = height

		vzoom = zoom
		
		' Store ratio...
		
		vratio = vheight / vwidth

		' Create global VirtualDisplay object...
		
		Display = Self
	
	End

	Method GetZoom:Float ()
		Return vzoom
	End
	
	Method SetZoom (zoomlevel:Float)
		If zoomlevel < 0.0 Then zoomlevel = 0.0
		vzoom = zoomlevel
	End
	
	Method AdjustZoom (amount:Float)
		vzoom = vzoom + amount
		If vzoom < 0.0 Then vzoom = 0.0
	End
	
	Method VMouseX:Float (limit:Bool)
		
		' Position of mouse, in real pixels, from centre of screen (centre being 0)...
		
		Local mouseoffset:Float = MouseX - Float (DeviceWidth) * 0.5
		
		' This calculates the scaled position on the virtual display. Somehow...
		
		Local x:Float = (mouseoffset / multi) / vzoom + (VDeviceWidth * 0.5)

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

		Local y:Float = (mouseoffset / multi) / vzoom + (VDeviceHeight * 0.5)
		
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

	Method UpdateVirtualDisplay (zoomborders:Bool, keepborders:Bool)

		' Store device resolution as float values to avoid loads of casts. Doing it here as
		' device resolution may potentially be changed on the fly on some platforms...
		
		fdw = Float (DeviceWidth)
		fdh = Float (DeviceHeight)
		
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

		Local sx:Float, sy:Float, sw:Float, sh:Float

		If zoomborders

			' Width/height of SCALED virtual display in real pixels...
			
			Local realx:Float = (vwidth * vzoom * multi)
			Local realy:Float = (vheight * vzoom * multi)
	
			' Space in pixels between real device borders and virtual device borders...
			
			Local offx:Float = (fdw - realx) * 0.5
			Local offy:Float = (fdh - realy) * 0.5

			' WIP: Retain borders when zoomed in... add option to lose borders...
			' Can keepborders check go outside?
			
			If keepborders

				If offx < widthborder
					sx = widthborder
					sw = fdw - widthborder * 2.0
				Else
					sx = offx
					sw = fdw - (offx * 2.0)
				Endif

			Else

				sx = offx
				sw = fdw - (offx * 2.0)

			Endif
			
			If keepborders
	
				If offy < heightborder
					sy = heightborder
					sh = fdh - heightborder * 2.0
				Else
					sy = offy
					sh = fdh - (offy * 2.0)
				Endif

			Else

				sy = offy
				sh = fdh - (offy * 2.0)

			Endif
			
			sx = Max (0.0, sx)
			sy = Max (0.0, sy)
			sw = Min (sw, fdw)
			sh = Min (sh, fdh)
			
			SetScissor sx, sy, sw, sh
			
		Else

			sx = Max (0.0, widthborder)
			sy = Max (0.0, heightborder)
			sw = Min (fdw - widthborder * 2.0, fdw)
			sh = Min (fdh - heightborder * 2.0, fdh)

			SetScissor sx, sy, sw, sh

		Endif
		
		' ---------------------------------------------------------------------
		' Scale and translate everything...
		' ---------------------------------------------------------------------
		
		Scale multi * vzoom, multi * vzoom

		' ---------------------------------------------------------------------
		' Shift display to account for borders/zoom level...
		' ---------------------------------------------------------------------

		If vzoom ' Gets skipped if zero...
		
			' Width and height of *scaled* virtual display in pixels...

			scaledw = (vwidth * multi * vzoom)
			scaledh = (vheight * multi * vzoom)

			' Find offsets by which view needs to be shifted...
			
			Local xoff:Float = (fdw - scaledw) * 0.5
			Local yoff:Float = (fdh - scaledh) * 0.5

			' Ahh, good old trial and error -- I have no idea how this works!
			
			xoff = (xoff / multi) / vzoom
			yoff = (yoff / multi) / vzoom
			
			' Aaaand, shift...
			
			Translate xoff, yoff
		
		Endif
		
	End

End
