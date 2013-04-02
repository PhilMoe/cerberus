'******************************************************************
'* Monkey port of Simple Verlet (physics) by grable
'* http://www.blitzbasic.com/codearcs/codearcs.php?code=1769
'* Port Author: Richard R Betson
'* 02/09/11
'* Language: monkey
'* Tagets: HTML5, FLASH, GLFW
'* License - Public Domain
'******************************************************************

Import mojo

Const S_TIMESTEP:Float = 0.1




Class TSPoint
	Field x:Float,y:Float ' current position
	Field oldx:Float,oldy:Float ' old position
	Field fx:Float,fy:Float ' impulse force
	Field mass:Float
	Field active:Int
	
	Function Create:TSPoint( x:Float,y:Float, mass:Float, active:Int = True)
		Local p:TSPoint = New TSPoint
		p.x = x
		p.y = y
		p.oldx = x
		p.oldy = y
		p.mass = mass
		p.active = active
		Return p
	End Function
	
	Method Update()
		If Not active Then Return
		
		Local tmpx1:Float = x
		Local tmpy1:Float = y		
		Local tmpx2:Float = fx * S_TIMESTEP * S_TIMESTEP
		Local tmpy2:Float = fy * S_TIMESTEP * S_TIMESTEP
		
		oldx = oldx+ tmpx2
		oldy = oldy+ tmpy2
		
		x = x- oldx
		y = y- oldy
		
		x = x+ tmpx1
		y = y+ tmpy1
		
		oldx = tmpx1
		oldy = tmpy1		
		
		fx = 0
		fy = 0
	End Method
	
	Method Render()
		SetColor 0,0,255
		DrawOval x-2,y-2, 5,5
	End Method
	
	Method Translate( x:Float,y:Float, reset:Int = False)
		Self.x = Self.x+ x
		Self.y = Self.y+ y		
		' reset movement
		If reset Then
			oldx = Self.x
			oldy = Self.y		
		Endif
	End Method
	
	Method Rotate( dir:Float, center:Float[], reset:Int = False)
		Local xr:Float = x - center[0]
		Local yr:Float = y - center[1]
		x = xr * Cos(dir) - yr * Sin(dir)
		y = xr * Sin(dir) + yr * Cos(dir)		
		x = x+ center[0]
		y = y+ center[1]
		' reset movement
		If reset Then
			oldx = x
			oldy = y
		Endif
	End Method
End Class


Class TSLink
	Field p1:TSPoint
	Field p2:TSPoint	
	Field restLength:Float
	Field k:Float	
	Field stress:Float
	
	Function Create:TSLink( p1:TSPoint, p2:TSPoint, k:Float)
		Local l:TSLink = New TSLink
		l.p1 = p1
		l.p2 = p2		
		l.k = k
		l.CalcRestLength()		
		Return l
	End Function
	
	Method Update()
		Local dx:Float = p1.x - p2.x
		Local dy:Float = p1.y - p2.y
		Local dist:Float = Sqrt( dx*dx + dy*dy)
		Local w:Float = p1.mass + p2.mass
		
		If p1.active Then
			p1.x = p1.x- ((dx / dist) * ((dist - restLength) * k)) * (p1.mass / w)
			p1.y = p1.y- ((dy / dist) * ((dist - restLength) * k)) * (p1.mass / w)
		Endif
		
		If p2.active Then
			p2.x = p2.x+ ((dx / dist) * ((dist - restLength) * k)) * (p2.mass / w)
			p2.y = p2.y+ ((dy / dist) * ((dist - restLength) * k)) * (p2.mass / w)
		Endif
		
		stress = (dist - restLength) / restLength
	End Method	
	
	Method Render()
		SetColor 255,255,255
		DrawLine p1.x,p1.y, p2.x,p2.y
	End Method
	
	Method CalcRestLength()
		restLength = Sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y))
	End Method	
End Class


Class TSGroup
	Field points:=New List<TSPoint>
	Field links:=New List<TSLink>	
	Field gravity:Float
	Field active:Int
	Field bbox:Float[4]
	Field center:Float[2]
	
	Function Create:TSGroup( gravity:Float = 0.0, active:Int = True)
		Local g:TSGroup = New TSGroup
		g.gravity = gravity
		Return g
	End Function
	
	Method AddPoint( p:TSPoint)
		If p Then points.AddLast( p)
	End Method
	
	Method AddLink( l:TSLink)
		If l Then links.AddLast( l)
	End Method	
	
	Method Update()
		If Not active Then Return
		
		For Local p:TSPoint = Eachin points
			p.fy = gravity
			p.Update()
		Next
		
		For Local l:TSLink = Eachin links
			l.Update()
		Next
		
		CalcBoundingBox()
		CalcCenterPoint()		
	End Method
	
	Method Render()
		For Local l:TSLink = Eachin links
			l.Render()
		Next
		
		For Local p:TSPoint = Eachin points
			p.Render()
		Next
		
		SetColor 0,192,0
		DrawFrame( bbox[0], bbox[1], bbox[2], bbox[3])
		
		SetColor 255,0,0
		DrawOval center[0]-2,center[1]-2,4,4
	End Method	
	
	Method Translate( x:Float,y:Float, reset:Int = False)
		For Local p:TSPoint = Eachin points
			p.Translate( x,y, reset)		
		Next
		CalcBoundingBox()
		CalcCenterPoint()
	End Method	
	
	Method Rotate( dir:Float, reset:Int = False)
		For Local p:TSPoint = Eachin points
			p.Rotate( dir, center, reset)
		Next
		CalcBoundingBox()
		CalcCenterPoint()
	End Method	
	
	Method CalcBoundingBox()
		bbox[0] = $FFFFFFF
		bbox[1] = $FFFFFFF
		bbox[2] = 0
		bbox[3] = 0
		For Local p:TSPoint = Eachin points
			bbox[0] = Min( bbox[0], p.x)
			bbox[1] = Min( bbox[1], p.y)
			bbox[2] = Max( bbox[2], p.x)
			bbox[3] = Max( bbox[3], p.y)
		Next
		bbox[2] = bbox[2]- bbox[0]
		bbox[3] = bbox[3]- bbox[1]
	End Method
	
	Method CalcCenterPoint()
		Local xtmp:Float,ytmp:Float, sz:Int = points.Count()
		For Local p:TSPoint = Eachin points
			xtmp = xtmp+ p.x
			ytmp = ytmp+ p.y
		Next
		center[0] = xtmp / sz
		center[1] = ytmp / sz		
	End Method
End Class



Function DrawFrame( x:Float,y:Float, w:Float,h:Float)	
	DrawLine x,y, x+w,y		' top
	DrawLine x,y+h, x+w,y+h	' bottom
	DrawLine x,y, x,y+h		' left
	DrawLine x+w,y, x+w,y+h	' right	
End Function

Function PointInRect:Int( px:Int,py:Int, rect:Int[])
	Return (px >= rect[0]) And (py >= rect[1]) And (px < rect[0] + rect[2]) And (py < rect[1] + rect[3])
End Function



 
Function Main()
	New Collision
End Function

Class Collision Extends App

Global obj1:TSGroup = TSGroup.Create( -5, False)
Const BOX_COEF:Float = 0.4
Const BOX_MASS:Float = 30
Global mb1:Int,mb2:Int
Global mx:Int,my:Int
Global mpoint:TSPoint
Global GHeight:Int=480
Global GWidth:Int=640

' globals


	Method OnCreate()
	SetUpdateRate(60)

	Local p1:TSPoint = TSPoint.Create( 0,0,	 BOX_MASS, True)
	Local p2:TSPoint = TSPoint.Create( 64,0,  BOX_MASS, True)
	Local p3:TSPoint = TSPoint.Create( 0,64,  BOX_MASS, True)
	Local p4:TSPoint = TSPoint.Create( 64,64, BOX_MASS, True)

	obj1.AddPoint( p1)
	obj1.AddPoint( p2)
	obj1.AddPoint( p3)
	obj1.AddPoint( p4)

	obj1.AddLink( TSLink.Create( p1, p2, BOX_COEF)) ' top
	obj1.AddLink( TSLink.Create( p2, p4, BOX_COEF)) ' right
	obj1.AddLink( TSLink.Create( p4, p3, BOX_COEF)) ' bottom
	obj1.AddLink( TSLink.Create( p3, p1, BOX_COEF)) ' left
	obj1.AddLink( TSLink.Create( p3, p2, BOX_COEF)) ' cross 1
	obj1.AddLink( TSLink.Create( p1, p4, BOX_COEF)) ' cross 2

	' move it some to the right
	obj1.Translate( 405,32, True) 
	' rotate it and give it some speed
	obj1.Rotate( 10)
	obj1.Translate( 4,0)


		
	End Method
	
	Method OnUpdate()
	If KeyHit( KEY_SPACE)	Then  obj1.active = Not obj1.active

' create points / links
	
	If KeyDown( KEY_Q) Or KeyDown( KEY_W) And (Not obj1.active) 
 
		 mx = MouseX()
		my = MouseY()	
		If TouchHit(0) And KeyDown(KEY_Q)
			' create point
			obj1.AddPoint( TSPoint.Create( mx,my, BOX_MASS, True))
		Else 
		If TouchHit(0) And KeyDown( KEY_W)
			' create link
			Local rect:Int[4]
			If mpoint = Null Then
				' select first point
				For Local p:TSPoint = Eachin obj1.points
					rect[0] = p.x - 4
					rect[1] = p.y - 4
					rect[2] = 8
					rect[3] = 8
					If PointInRect( mx,my, rect) Then
						mpoint = p
						Exit
					Exit
					Endif
				Next
		'endif
			Else
				' select second point 
				For Local p:TSPoint = Eachin obj1.points
					rect[0] = p.x - 4
					rect[1] = p.y - 4
					rect[2] = 8
					rect[3] = 8
					If PointInRect( mx,my, rect) Then
						obj1.AddLink( TSLink.Create( mpoint, p, BOX_COEF))
						'Exit
					Exit
					Endif
				Next
				mpoint = Null
			Endif
		Endif
		Endif
	'	FlushMouse()		
	Else
' move single point / modify link
		If TouchDown(0) Then
			mx = MouseX()
			my = MouseY()
			If Not mb1 Then
				' select point
				Local rect:Int[4]
				For Local p:TSPoint = Eachin obj1.points
					rect[0] = p.x - 4
					rect[1] = p.y - 4
					rect[2] = 8
					rect[3] = 8
					If PointInRect( mx,my, rect) Then
						mpoint = p					
					'	Exit
					Exit
					Endif
				Next
				mb1 = True
			Endif
			' modify point
			If mpoint Then
				mpoint.x = mx
				mpoint.y = my
				' modify connected links
				If Not obj1.active Then
					' search for links with this point
					For Local l:TSLink = Eachin obj1.links
						If (l.p1 = mpoint) Or (l.p2 = mpoint) Then
							l.CalcRestLength()
						Endif
					Next
					' cancel allow movement
					mpoint.oldx = mpoint.x
					mpoint.oldy = mpoint.y
				Endif
			Endif
		Else
			If mb1 Then
				' reset
				mb1 = False
				mpoint = Null
			'	FlushMouse()
			Endif
		Endif
	Endif

' turn point on/off	
	If KeyHit( KEY_A) Then
		If mpoint <> Null Then
			mpoint.active = Not mpoint.active
		Endif
	Endif
	
' rotate box
	If KeyDown( KEY_1) 
		obj1.Rotate( -0.5, False)
	Else
	If KeyDown( KEY_2) Then
		obj1.Rotate( 0.5, False)
	Endif
	Endif	
	
	obj1.Update()
	
	For Local p:TSPoint = Eachin obj1.points
		' bottom
		If p.y > GHeight 'Then 
			p.y = GHeight
			' full friction
			p.oldx = p.x
		Endif
		' left, right
		If p.x < 0 'Then
			p.x = 0
		Else
			If p.x > GWidth 
			p.x = GWidth
		Endif
		Endif
	Next		

	
	End Method
	
	Method OnRender()
	'Scale DeviceWidth/640.0,DeviceHeight/480.0
	Cls
	PushMatrix()


	obj1.Render()

	' some help
	SetColor 255,255,255
	DrawText "HELP:", 0,0
	DrawText "  Pause Simulation/Edit mode: SPACE", 0,15	
	DrawText "  Rotate Left/Right:  1 / 2", 0,30
	DrawText "  Modify Point: MB-1 + DRAG", 0,45
	DrawText "  Create Point: Q", 0,60
	DrawText "  Create Link: W (select 2 points)", 0,75
	DrawText "  Turn point On/Off: A (on selected point)", 0,90

	PopMatrix()
	

		
	End Method
	
	
	
End Class












