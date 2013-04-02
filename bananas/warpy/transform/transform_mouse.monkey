'Map panning, rotating and zooming all using easy matrices
'by Christian Perfect

'By saving a transformation matrix and using the Translate, Rotate and Scale operations on it, we can manipulate the player's view of a map very easily.
'The InvTransform command allows you to do the transformation backwards, converting screen co-ordinates to map co-ordinates

Import mojo

Class TransformMapApp Extends App
	Field mx#,my#			'current screen co-ordinates of mouse
	Field tmx#,tmy#		'current map co-ordinates of mouse
	Field omx#,omy#		'previous map co-ordinates of mouse
	
	Field mapMatrix#[]	'the transformation matrix applied to the map

	Method OnCreate()
		SetUpdateRate 60
		
		'I want the map to be size 400x300 and to fill the entire screen when the program starts.
		'So, the first step is to Scale the screen and grab the resulting transformation matrix 
		PushMatrix
			Scale DeviceWidth()/400.0, DeviceHeight()/300.0	
			mapMatrix = GetMatrix()
		PopMatrix
		'The Push/PopMatrix isn't strictly necessary here since the matrix gets reset when the program first renders, but it's good to get in the habit of using it
	End
	
	Method OnUpdate()
		Local coords#[]
	
		'get the mouse's screen co-ordinates
		mx = MouseX()
		my = MouseY()

		'We're going to do some transformations on the mapMatrix, and they need to be undone so they don't affect drawing, so we need a Push/PopMatrix round all this code
		PushMatrix
			
			'Translate to the mouse's screen co-ords. This way, the rotation and scaling are centred on the mouse.
			Translate mx,my
			
			Rotate (KeyDown(KEY_LEFT)-KeyDown(KEY_RIGHT))*1.5
			
			Local s# = 1+(KeyDown(KEY_UP) - KeyDown(KEY_DOWN))*.01
			Scale s,s
			
			'Now move back. While other points on the map might have been transformed, the mouse's position remains unchanged.
			Translate -mx,-my


			'Apply the last saved mapMatrix, that is, all rotations, scalings and translations applied since the program started
			Transform mapMatrix[0],mapMatrix[1],mapMatrix[2],mapMatrix[3],mapMatrix[4],mapMatrix[5]

			'Work out what map co-ordinate the mouse is pointing at by doing the matrix transformation backwards.
			coords = InvTransform([mx,my])
			tmx = coords[0]
			tmy = coords[1]
			
			
			If TouchDown(0)
				'Pan the map based on how far the mouse has moved since last frame
				Translate tmx-omx, tmy-omy	
			Endif

			mapMatrix = GetMatrix()		'Save the new map transformation matrix, preserving all the transformations we've just done.
			
			'Work out the mouse's map co-ordinates based on the new matrix.
			coords = InvTransform([mx,my])
			omx = coords[0]
			omy = coords[1]
			
		PopMatrix
	
	End
	
	Method OnRender()
		Cls

		SetColor 255,255,255
		SetFont Null
	
		PushMatrix
		
		'apply the saved map transformation matrix
		Transform mapMatrix[0],mapMatrix[1],mapMatrix[2],mapMatrix[3],mapMatrix[4],mapMatrix[5]
		
		'draw the map
		drawMap
		
		'Draw the mouse's map co-ordinates above the cursor
		DrawText Int(omx)+","+Int(omy),omx,omy-TextHeight()
		Local bits#[] = InvTransform([mx,my])
		DrawText Int(bits[0])+","+Int(bits[1]),omx,omy-TextHeight()*2
			
		
		PopMatrix
	
		DrawText "Click and drag to move",0,0
		DrawText "Arrow keys to rotate and zoom",0,TextHeight()
		
	End
	
	'draw the map: draw a grid of 100x100 boxes, with their centres labelled
	Method drawMap()
		For Local x=0 To 400 Step 100
			DrawLine x,0,x,300
		Next
		For Local y=0 To 300 Step 100
			DrawLine 0,y,400,y
		Next
		
		For Local x=50 To 400 Step 100
			For Local y=50 To 300 Step 100
				DrawCircle x,y,2
				DrawText x+","+y,x+2,y+2
			Next
		Next
	End
End

Function Main()
	New TransformMapApp
End