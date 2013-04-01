'Draw bezier curves

'A bezier curve is defined by four control points. 
'The first control point is the start of the curve.
'The second control point defines the initial direction of the curve (relative to the start)
'The third control point defines the final direction of the curve (relative to the end point)
'The fourth control point is the end of the curve.

'The curve is parameterised by a variable t. That means, any value of t between 0 and 1 corresponds to a point on the curve.
't=0 corresponds to the start of the curve, and t=1 corresponds to the end of the curve.

'We can only draw an approximation to the bezier curve for various difficult maths reasons.
'There are a few algorithms for drawing bezier curves. 
'This one works by working out the positions of points on the curve for various values of t, and joining them up with straight lines.


Import mojo


'The bezier class represents a single bezier curve.
'It is defined by four control points, which are stored in the points array
'inc is the 'step size' used for drawing - a smaller number means a more accurate curve
Class bezier
	Field points#[]
	Field inc#
	
	Method New(points#[],inc#=-1)
		Self.points = points
		
		If inc=-1	'if inc is not given, make a guess based on the distance between the two end points
			Local dx#=points[6]-points[0]
			Local dy#=points[7]-points[1]
			Local d#=Sqrt(dx*dx+dy*dy)
			inc=1/d
		Endif
		Self.inc = inc
	End
	
	Method xpos#(t#)
		'work out the x co-ordinate of the point on the curve corresponding to the given value of t
		Local nt#=1-t
		Return nt*nt*nt*points[0] + 3*nt*nt*t*points[2] + 3*nt*t*t*points[4] + t*t*t*points[6]
	End Method
	
	Method ypos#(t#)
		'work out the y co-ordinate of the point on the curve corresponding to the given value of t
		Local nt#=1-t
		Return nt*nt*nt*points[1] + 3*nt*nt*t*points[3] + 3*nt*t*t*points[5] + t*t*t*points[7]
	End Method
	
	Method draw()
		'draw the control points
		SetColor 0,0,255
		DrawPoints points
		'draw lines showing how the second and third control points define the direction of the curve
		SetAlpha .3
		DrawLine points[0],points[1],points[2],points[3]
		DrawLine points[4],points[5],points[6],points[7]
		SetAlpha 1
		
		'draw the curve
		'it will work by increasing t from 0 to 1, 
		'calculating the co-ordinates of the corresponding point on the curve at each step,
		'and drawing a line between it and the previous calculated point 
		SetColor 255,255,255
		Local ox#=points[0], oy#=points[1]	'the first point on the curve is just the first control point
		
		Local x#,y#
		Local t#=0
		
		While t<1
			t+=inc		'add inc to t, so moving along the curve a little bit
			If t>1 t=1
			
			'calculate the position of the corresponding point on the curve
			x=xpos(t)	
			y=ypos(t)
			
			'draw a line between the previous point and the point we just calculated
			DrawLine ox,oy,x,y
			
			'the point we just calculated now becomes the 'previous' point
			ox=x
			oy=y
		Wend
	End
End

'Draw a circle at each of the points in the given array
Function DrawPoints(points#[])
	For Local i=0 To points.Length-1 Step 2
		If points[i]>0
			DrawCircle points[i],points[i+1],3
		Endif
	Next
End

Class BezierApp Extends App
	Field points#[8],i			'This array stores the control points the user is currently placing
	Field beziers:List<bezier>		'This list will store all the beziers that have been drawn
	
	Method OnCreate()
		SetUpdateRate 10
		beziers = New List<bezier>
	End
	
	Method OnUpdate()
	
		If JoyHit( JOY_BACK ) Error ""
		
		'when the user clicks, place a control point
		If TouchHit(0)
			points[i]=TouchX()	'store where the user clicked
			points[i+1]=TouchY()
			i+=2
			If i=8				'if four points have been placed, we can create a bezier
				beziers.AddLast(New bezier(points))
				
				points = New Float[8]	're-initialise the points array
				i=0
			Endif
		Endif
	End
	
	Method OnRender()
		Cls
		
		'draw all the beziers that have been created
		For Local b:bezier=Eachin beziers
			b.draw
		Next
		
		'draw the control points the user is currently placing
		SetColor 255,255,0
		DrawPoints points

		SetColor 255,255,255
		DrawText "Click to place control points",0,0
	End
End

Function Main()
	New BezierApp
End