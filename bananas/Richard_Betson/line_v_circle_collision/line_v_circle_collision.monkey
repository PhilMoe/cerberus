'******************************************************************
'* Line vs. Circle Collision Example							  
'* Author - Richard R Betson
'* 01/25/2010
'* Language: monkey
'* Targets: HTML5, FLASH, GLFW
'* License - Public Domain
'******************************************************************

Import mojo

Global line_list:=New List<Lines>
Global circle_list:=New List<CirclesL>
Global line_nx#	'Used by LineToCircle
Global line_ny#
Global IntersectX#, IntersectY#	'Used by NearestPointInLine

Global total_circles=10 'Total Circles
Global dh=1
Global dw=1
Global fps,fp,fps_t
 
Function Main()
	New Collision
End Function

Class Lines
Field x1,y1,x2,y2
	Method New(Lx1,Ly1,Lx2,Ly2)
		Self.x1=Lx1
		Self.y1=Ly1
		Self.x2=Lx2
		Self.y2=Ly2
	End Method
End Class

Class CirclesL
Field x1#,y1#,rad,dx#,dy#
	Method New(Lx1,Ly1,rad1,dx1#=0,dy1#=0)
		Self.x1=Lx1
		Self.y1=Ly1
		Self.rad=rad1
		'Seed=Millisecs()
		Self.dx=2.0+Rnd(-1.0,4.0)
		Self.dy=2.0+Rnd(-1.0,4.0)
	End Method
End Class


Class Collision Extends App

Field img:Image


	Method OnCreate()
		SetUpdateRate(60)
		
		dw=DeviceWidth()
		dh=DeviceHeight()
		'dw=320
		'dh=440
		Local  linej:Lines=New Lines(0,0,dw,0)
		line_list.AddLast linej
		linej=New Lines(dw,1,dw,dh-1)
		line_list.AddLast linej
		linej=New Lines (0,dh,dw,dh)
		line_list.AddLast linej
		linej=New Lines (0,1,0,dh-1)
		line_list.AddLast linej
		


		linej=New Lines(160,100,460,100)
		line_list.AddLast linej
		linej=New Lines(461,101,461,139)
		line_list.AddLast linej
		linej=New Lines (160,140,460,140)
		line_list.AddLast linej
		linej=New Lines (159,101,159,139)
		line_list.AddLast linej

		linej=New Lines (270,400,340,270)
		line_list.AddLast linej
		linej=New Lines (270,400,410,400)
		line_list.AddLast linej
		linej=New Lines (340,270,410,400)
		line_list.AddLast linej

		
		Local i
		For i=1 To total_circles
			Local  c:CirclesL=New CirclesL(20+Rnd(20),50+Rnd(50),8)
			circle_list.AddLast c
		Next
		
		Image.DefaultFlags=Image.MidHandle
		img=LoadImage("circle2f.png")

	End Method
	
	Method OnUpdate()
		fps=fps+1
		If fps_t<Millisecs()
		fp=(fps)
		fps_t=1000+Millisecs()
		fps=0
		Endif
		
	End Method
	
	Method OnRender()

'		fps=fps+1
'		If fps_t<Millisecs()
'		'fps2=fps
'		fp=(fps)
'		fps_t=1000+Millisecs()
'		fps=0
'		EndIf



	
	Cls 0,0,0
	For Local circle:CirclesL=Eachin circle_list

	PushMatrix()

	Local sw,lnc
	Local dist#
	Local lines:Lines
	For lines=Eachin line_list
		'Check for line collision
		lnc=LineToCircle(lines.x1,lines.y1,lines.x2,lines.y2,circle.x1,circle.y1,circle.rad)
		
		
		If lnc=1
			'Reverse direction
			Local circledx#=-circle.dx
			Local circledy#=-circle.dy
			'Back up clear of the line
			While LineToCircle(lines.x1,lines.y1,lines.x2,lines.y2,circle.x1,circle.y1,circle.rad+.1)=1
				circle.x1=circle.x1+circledx
				circle.y1=circle.y1+circledy
			Wend

			Local old_dst#=100
			Local lines3:Lines=New Lines ' Find the nearest line point
			For Local lines2:Lines=Eachin line_list
					
				Local dst#= Abs( (DistanceToLineSegment (lines2.x1,lines2.y1,lines2.x2,lines2.y2,circle.x1,circle.y1) ))

				'Local dst#= Abs( (NearestPointInLine (lines2.x1,lines2.y1,lines2.x2,lines2.y2,circle.x1,circle.y1) ))

				If dst=old_dst 'Then Print "kk"
					circle.dx=-circle.dx
					circle.dy=-circle.dy
					'lines3=lines2
					old_dst=dst
					sw=0
				Endif
				
				If dst<old_dst
					lines3=lines2
					old_dst=dst
					sw=1
				
				Endif
			Next

			If sw=1	
			LineRecalc( lines3.x1,lines3.y1,lines3.x2,lines3.y2 ) 'Calc Normal		
			Local dot#=circle.dx*line_nx+circle.dy*line_ny ' Dot Product
			'Print line_ny
			circle.dx=circle.dx-2.0*line_nx*dot 'Calc new DX/DY direction
			circle.dy=circle.dy-2.0*line_ny*dot
			
			circle.x1=(circle.x1+circle.dx*.95) ' Move - Friction/Braking
			circle.y1=(circle.y1+circle.dy*.95)
			Exit
			Endif
	
		Endif
	Next
	If sw=0 
		circle.x1=circle.x1+circle.dx
		circle.y1=circle.y1+circle.dy
	Endif

		PopMatrix
		SetBlend 0
		DrawImage(img,circle.x1,circle.y1)
	Next

		SetColor (255,0,0)
		DrawLine(160,100,460,100)
		DrawLine(460,101,460,139)
		DrawLine(160,140,460,140)
		DrawLine(160,101,160,139)
		
		DrawLine(270,400,340,270)
		DrawLine(271,400,409,400)
		DrawLine(341,271,410,400)
	
	End Method

	Function LineToCircle:Float( lx1#, ly1#, lx2#, ly2#, cx#, cy#, r#)
	'This function by Jeppe Nielsen (Public Domain)
	'http://www.blitzbasic.com/codearcs/codearcs.php?code=998
	
		Local dx# = lx2 - lx1
		Local dy# = ly2 - ly1
		Local ld# = Sqrt((dx*dx) + (dy*dy))
		Local lux# = dx / ld
		Local luy# = dy / ld
		Local lnx# = luy
		Local lny# = -lux
		Local dx1# = cx - (lx1 - lux*r)
		Local dy1# = cy - (ly1 - luy*r)
		Local d# = Sqrt((dx1*dx1) + (dy1*dy1))
		dx1 = dx1 / d
		dy1 = dy1/ d
		Local dx2# = cx - (lx2 + lux * r)
		Local dy2# = cy - (ly2 + luy*r)
		d = Sqrt((dx2*dx2) + (dy2*dy2))
		dx2 = dx2  / d
		dy2 = dy2 / d
		Local dot1# = (dx1 * lux) + (dy1 * luy)
		Local dot2# = (dx2 * lux) + (dy2 * luy)
		Local px#=lx1-cx
		Local py#=ly1-cy
		Local distsq# = Abs((dx * py - px * dy)  / ld )
	
	'You can get point of collision using these two variables (make them global)
	'Local LineColX# = cx - lnx * Sqrt(distsq) 
	'Local LineColY# = cy - lny * Sqrt(distsq)
	
	
		 If (( dot1>=0.0 And dot2<=0.0) Or (dot1<=0.0 And dot2>=0.0)) And (distsq<=r)
			Return 1.0
		Endif
	
	End Function 
	
	Function LineRecalc(line_x1#,line_y1#,line_x2#,line_y2#)
	'This function by Jeppe Nielsen and Braincell(Public Domain)
	'http://www.blitzbasic.com/codearcs/codearcs.php?code=998
	
			Local line_dx#=line_x2-line_x1
			Local line_dy#=line_y2-line_y1
		
			Local line_d#=Sqrt(line_dx*line_dx+line_dy*line_dy)
			'Print line_d
				If line_d<0.0001
					line_d=0.0001
				Endif
		'Print line_d
		'Print line_dx
			Local line_ux#=line_dx/line_d
			Local line_uy#=line_dy/line_d
		
			line_nx=line_uy
			line_ny=-line_ux
	
	End Function
	
	
	Function Orientation% ( x1#,y1#, x2#,y2#, Px#,Py# )
		'http://www.blitzbasic.com/codearcs/codearcs.php?code=2180 - Public Domain Function Source 
		'
		' Linear determinant of the 3 points.
		' This function returns the orientation of px,py on line x1,y1,x2,y2.
		' Look from x2,y2 to the direction of x1,y1.
		' If px,py is on the right, function returns +1
		' If px,py is on the left, function returns -1
		' If px,py is directly ahead or behind, function returns 0
		Return Sgn((x2 - x1) * (Py - y1) - (Px - x1) * (y2 - y1))
	End Function
	
	
	Function IntersectPoint ( x1#,y1#, x2#,y2#, x3#,y3#, x4#,y4# )
		'http://www.blitzbasic.com/codearcs/codearcs.php?code=2180 - Public Domain Function Source
		'
		'Function returns the X,Y position of the two intersecting lines.
		'IntersectX and IntersectY are global variables of the main program.
		'The lines are infinite, is line1 goes through x1,y1,x2,y2 and line2 goes through x3,y3,x4,y4.
		'For line segments you must check if the lines truly intersect with the function Intersect% before you use this.
		
		Local dx1# = x2 - x1
		Local dx2# = x4 - x3
		Local dx3# = x1 - x3
		
		Local dy1# = y2 - y1
		Local dy2# = y1 - y3
		Local dy3# = y4 - y3
		
		Local R# = dx1 * dy3 - dy1 * dx2
		
		If R <> 0 Then
			R  = (dy2 * (x4 - x3) - dx3 * dy3) / R
			IntersectX = x1 + R * dx1
			IntersectY = y1 + R * dy1
		Else
			If (((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) = 0) Then
				IntersectX = x3 
				IntersectY = y3
			Else
				IntersectX = x4
				IntersectY = y4
			Endif
		Endif
		
	End Function

	Function DistanceToLineSegment# ( x1#,y1#, x2#,y2#, Px#,Py# )
		' This function calculates the distance between a line segment and a point.
		' So this function is useful to determine if line intersects a circle.
		' To also determine the point on the line x1,y1,x2,y2 which is the closest to px,py , use function NearestPointInLine#
		
		Local Dx#, Dy#, Ratio#
		
		If (x1 = x2) And (y1 = y2) Then
			Return Sqrt( (Px-x1)*(Px-x1)+(Py-y1)*(Py-y1) )
		Else
			
			Dx    = x2 - x1
			Dy    = y2 - y1
			Ratio = ((Px - x1) * Dx + (Py - y1) * Dy) / (Dx * Dx + Dy * Dy)
			
			If Ratio < 0 Then
				Return Sqrt( (Px-x1)*(Px-x1)+(Py-y1)*(Py-y1) )
			Else If Ratio > 1
				Return Sqrt( (Px-x2)*(Px-x2)+(Py-y2)*(Py-y2) )
			Else
				Return Sqrt ((Px - ((1 - Ratio) * x1 + Ratio * x2))*(Px - ((1 - Ratio) * x1 + Ratio * x2))+(Py - ((1 - Ratio) * y1 + Ratio * y2))*(Py - ((1 - Ratio) * y1 + Ratio * y2)))
			Endif
			
		Endif
		
	End Function

	
	Function NearestPointInLine# ( lx1#,ly1#, lx2#,ly2#, x#,y# )
		'Public Domain Function by Jasu
		'http://www.blitzbasic.com/codearcs/codearcs.php?code=2180
		'
		' This function calculates the point between lx1,ly1 and lx2,ly2 which is the nearest to x,y.
		' Result is put in global variables IntersectX,IntersectY
		' Function also returns the distance between x,y and the calculated point.
			
		Local dx#=lx2-lx1
		Local dy#=ly2-ly1
		'd# = Sqrt(dx*dx+dy*dy)
		'ux# = dx/d
		'uy# = dy/d
		Local Ori1% = Orientation(lx1,ly1, (lx1+dy),(ly1-dx), x,y)
		Local Ori2% = Orientation(lx2,ly2, (lx2+dy),(ly2-dx), x,y)
		If (Ori1 = 1 And Ori2 = 1) Or Ori2 = 0 Then
			IntersectX = lx2
			IntersectY = ly2
		Else If (Ori1 = -1 And Ori2 = -1) Or Ori1 = 0
			IntersectX = lx1
			IntersectY = ly1
		Else
			IntersectPoint( lx1,ly1, lx2,ly2, x,y, x+dy,y-dx )
		Endif
		Return Sqrt((x-IntersectX)*(x-IntersectX)+(y-IntersectY)*(y-IntersectY))
		
	End Function

End Class







