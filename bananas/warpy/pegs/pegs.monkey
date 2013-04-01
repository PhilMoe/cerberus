
Import mojo

Class Level
	Field points#[]
	Field bounds[]
	Field pegs#[]
	
	Method New(_points#[],_bounds[],_pegs#[])
		
		points = _points
		bounds = _bounds
		pegs = _pegs
	End
End

Class Point
	Field x#,y#
	
	Method New(_x#,_y#)
		x=_x
		y=_y
	End
	
	Method Draw()
		SetColor 255,255,255
		DrawCircle x,y,2.5
	End
End

Class Bound
	Field s:Point,e:Point
	
	Field conflicts
	
	Method New(_s:Point,_e:Point)
		s=_s
		e=_e
	End
	
	Method Draw()
		If conflicts
			SetColor 255,0,0
		Else
			SetColor 255,255,255
		Endif
		conflicts = False
		
		If bapp.dragbound = Self
			DrawLine s.x,s.y,bapp.mx,bapp.my
			DrawLine bapp.mx,bapp.my,e.x,e.y
		Else
			DrawLine s.x,s.y,e.x,e.y
		End
	End
	
	Function contains(x#,y#)
		Local n=0
		For Local b:Bound=Eachin bapp.bounds
			If linesintersect(b.s,b.e,-500,0,x,y)>=0
				n+=1
			Endif
		Next
		Return (n Mod 2)=1
	End
	Function contains(pt:Point)
		Return contains(pt.x,pt.y)
	End
End

Class Peg Extends Point
	Field in
	
	Method New(_x#,_y#, _in)
		x=_x
		y=_y
		in=_in
	End
	
	Method Draw()
		If in
			SetColor 0,255,0
			DrawCircle x,y,5
		Else
			SetColor 255,0,0
			DrawRect x-5,y-5,10,10
		Endif
		
		If Bound.contains(Self)
			SetColor 0,0,0
			DrawCircle x,y,1
		Endif
	End
End

Class BoingApp Extends App
	Field screenMatrix#[]
	
	Field levels:List<Level>

	Field points:List<Point>
	Field bounds:List<Bound>
	Field pegs:List<Peg>
	
	Field state$="normal"
	Field moves
	Field ox#,oy#,mx#,my#
	
	Field dragdx#,dragdy#,dragox#,dragoy#,dragallowed
	Field dragpoint:Point
	Field dragbound:Bound
	
	Field level

	Method OnCreate()
		SetUpdateRate 60
		
		levels = New List<Level>
		
		levels.AddLast (New Level(
										[-50.0,-50.0,-50.0,50.0,50.0,-50.0,50.0,50.0],
										[0,1,0,2,1,3,2,3],
										[-74.0,-99.0,1.0, -145.0,-69.0,1.0, -153.0,-6.0,1.0, -145.0,46.0,1.0, -87.0,84.0,1.0, -93.0,-53.0,0.0, -96.0,25.0,0.0]))
		
		levels.AddLast (New Level( 
										[-180.0,-140.0,180.0,-140.0,0.0,140.0],
										[0,1,1,2,2,0],
										[-149.0,-122.0,1.0, 131.0,-117.0,1.0, 121.0,104.0,1.0, -146.0,122.0,1.0, -7.0,-8.0,0.0]))
		
		levels.AddLast (New Level(
										[-50.0,-50.0,-50.0,50.0,50.0,-50.0,50.0,50.0],
										[0,1,0,2,1,3,2,3],
										[-124.0,51.0,1.0, -99.0,-38.0,1.0, -47.0,-84.0,1.0, 54.0,-83.0,1.0, 108.0,-34.0,1.0, 130.0,45.0,1.0, 3.0,1.0,1.0, -42.0,28.0,0.0, 41.0,26.0,0.0]))
		
		changeLevel

		PushMatrix
		Scale DeviceWidth()/400.0,DeviceHeight()/300.0
		Translate 200,150
		screenMatrix = GetMatrix()
		PopMatrix

	End
	
	Method changeLevel()
		Local l:Level = levels.RemoveFirst()
		levels.AddLast l
		
		points=New List<Point>
		Local pointsarr:Point[l.points.Length/2]

		For Local i=0 To l.points.Length-1 Step 2
			Local pt:Point = New Point(l.points[i],l.points[i+1])
			points.AddLast pt
			pointsarr[i/2] = pt
		Next

		bounds = New List<Bound>
		For Local i=0 To l.bounds.Length-1 Step 2
			Local b:Bound = New Bound( pointsarr[l.bounds[i]], pointsarr[l.bounds[i+1]] )
			bounds.AddLast b
		Next
		
		pegs = New List<Peg>
		For Local i=0 To l.pegs.Length-1 Step 3
			pegs.AddLast( New Peg(l.pegs[i],l.pegs[i+1], l.pegs[i+2]) )
		Next
		
		moves = 0
		
		setState "normal"

	End
	
	Method OnUpdate()
		
		PushMatrix
		Transform screenMatrix[0],screenMatrix[1],screenMatrix[2],screenMatrix[3],screenMatrix[4],screenMatrix[5]
		mx = realMouseX()
		my = realMouseY()
		PopMatrix
		
		Select state
		Case "normal"
			If TouchDown(0)
				setState "startdrag"
			Endif
		Case "startdrag"
			If TouchDown(0)
				getDragBound
			Else
				setState "normal"
			Endif
		Case "dragbound"
			checkDragBound
			
			If TouchDown(0)
				If Sgn((mx-ox)*dragdx + (my-oy)*dragdy)=-1 And linesintersect(dragbound.s,dragbound.e,ox,oy,mx,my)>=0
					setState "startdrag"
				Endif
			Else
				If dragallowed
					bounds.RemoveEach dragbound
					Local mid:Point=New Point(mx,my)
					points.AddLast mid
					bounds.AddLast New Bound(dragbound.s,mid)
					bounds.AddLast New Bound(mid,dragbound.e)
				Endif
				setState "normal"
			Endif
		Case "dragpoint"
			checkDragPoint
			
			If TouchDown(0)
				dragpoint.x = mx + dragdx
				dragpoint.y = my + dragdy
			Else
				If Not dragallowed
					dragpoint.x = dragox
					dragpoint.y = dragoy
				Endif
				setState "normal"
			Endif
		End
		
		If TouchHit(0)
'			Print Int(mx)+","+Int(my)+",0, "
		Endif
		
		ox = mx
		oy = my
	End
	
	Method checkDragBound()
		dragallowed=True
		For Local b:Bound=Eachin bounds
			If b<>dragbound
				Local l1#=linesintersect(b.s,b.e,dragbound.s.x,dragbound.s.y,mx,my)
				Local l2#=linesintersect(b.s,b.e,mx,my,dragbound.e.x,dragbound.e.y)
				If (l1>0.00001 And l1<0.99999) Or (l2>0.00001 And l2<0.99999)
					dragallowed=False
					b.conflicts=True
					dragbound.conflicts=True
				Endif
			Endif
		Next
	End
	
	Method checkDragPoint()
		dragallowed=True
		For Local b:Bound=Eachin bounds
			If b.s=dragpoint Or b.e=dragpoint
				For Local b2:Bound=Eachin bounds
					If b<>b2
						Local lambda#=linesintersect(b.s,b.e,b2.s,b2.e)
						If lambda>0.00001 And lambda<0.99999
							dragallowed=False
							b.conflicts=True
							b2.conflicts=True
						Endif
					Endif
				Next
			Endif
		Next
	End
	
	Method setState(_state$)
		Local ostate$=state
		state = _state
		
		Select state
		Case "normal"
			dragbound = Null
			dragpoint = Null
			If ostate="dragpoint" Or ostate="dragbound"
				moves+=1
			Endif
			checkPegs
			
		Case "startdrag"
			dragbound = Null
			dragpoint = Null
			If ostate = "normal"
				getDragPoint
			Endif
		End

	End
	
	Method getDragPoint()
		For Local pt:Point=Eachin points
			Local dx#=pt.x-mx
			Local dy#=pt.y-my
			If dx*dx+dy*dy<50
				dragpoint = pt
				dragdx=pt.x-mx
				dragdy=pt.y-my
				dragox=pt.x
				dragoy=pt.y
				setState "dragpoint"
				Exit
			Endif
		Next
	End
	
	Method getDragBound()
		For Local b:Bound=Eachin bounds
			If linesintersect(b.s,b.e,ox,oy,mx,my)>=0
				dragbound=b
				dragdx = mx-ox
				dragdy = my-oy
				setState "dragbound"
				Return
			End
		Next
	End
	
	Method checkPegs()
		For Local p:Peg=Eachin pegs
			If Bound.contains(p)<>p.in
				Return
			Endif
		Next
		Print "OK!"
		If moves=1
			Print "Did it in 1 move!!!"
		Else
			Print "Dit it in "+moves+" moves"
		Endif
		changeLevel
	End
	
	Method OnRender()
		Cls

		SetColor 255,255,255
		
		PushMatrix

		Transform screenMatrix[0],screenMatrix[1],screenMatrix[2],screenMatrix[3],screenMatrix[4],screenMatrix[5]
		
		For Local p:Peg = Eachin pegs
			p.Draw
		Next
		
		For Local b:Bound=Eachin bounds
			b.Draw
		Next
		
		For Local pt:Point=Eachin points
			pt.Draw
		Next
		
		PopMatrix

		If moves=1
			DrawText "1 move",0,0
		Else
			DrawText moves+" moves",0,0
		Endif
		
	End
End

Function realMouseX#()
	Local parts#[] = InvTransform([MouseX(),MouseY()])
	Return parts[0]
End
Function realMouseY#()
	Local parts#[] = InvTransform([MouseX(),MouseY()])
	Return parts[1]
End

'simple version
Function linesintersect#(ax#,ay#,bx#,by#,cx#,cy#,dx#,dy#,fit=0)
	'fit, bitmask, set:
	' 1: doesn't need to be on first segment
	' 2: doesn't need to be on second segment
	bx-=ax
	by-=ay
	dx-=cx
	dy-=cy
	
	Local lambda#,mu#
	
	If dx<>0
		lambda=(cy-ay+(ax-cx)*dy/dx)/(by-bx*dy/dx)
	Else
		lambda=(cx-ax+(ay-cy)*dx/dy)/(bx-by*dx/dy)
	Endif
	If bx<>0
		mu=(ay-cy+(cx-ax)*by/bx)/(dy-dx*by/bx)
	Else
		mu=(ax-cx+(cy-ay)*bx/by)/(dx-dy*bx/by)
	Endif
	
	If (lambda>=0 And lambda<=1) Or (fit & 1)
	 If (mu>=0 And mu<=1) Or (fit & 2)
		Return lambda
	 Endif
	Endif
	
	Return -1
End Function

Function linesintersect#(s:Point,e:Point,x1#,y1#,x2#,y2#)
	Return linesintersect(s.x,s.y,e.x,e.y,x1,y1,x2,y2)
End
Function linesintersect#(s1:Point,e1:Point,s2:Point,e2:Point)
	Return linesintersect(s1.x,s1.y,e1.x,e1.y,s2.x,s2.y,e2.x,e2.y)
End

Global bapp:BoingApp
Function Main()
	bapp = New BoingApp
End
