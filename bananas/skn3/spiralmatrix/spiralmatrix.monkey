
Import mojo

Function Main()
	Local app := New MyApp
End

Class MyApp Extends App
	Method OnCreate()
		
		Local segment:Segment
		
		segment = New Segment(20,460,255,0,0,Null)
		
		Local flag := False
		For Local count := 0 To 30
			If flag = False
				segment = New Segment(0,0,255,255,255,segment)
				flag = True
			Else
				segment = New Segment(0,0,255,0,0,segment)
				flag = False
			End
		Next
		
		SetUpdateRate(60)
	End
	
	Method OnRender()
		Cls(0,0,0)
		For Local segment := Eachin Segment.root
			segment.Render()
		Next
		Segment.rotation += 0.01
	End
End

Class Segment
	Global root := New List<Segment>
	Global rotation := 0.0
	Global counter := 1.0
	
	Field parent:Segment
	Field children := New List<Segment>
	Field x:Float
	Field y:Float
	Field r:Int
	Field g:Int
	Field b:Int
	Field multiplier := 1.0
	
	Method New(x:Float,y:Float,r:Int,g:Int,b:Int,parent:Segment)
		Self.x = x
		Self.y = y
		Self.r = r
		Self.g = g
		Self.b = b
		Self.multiplier = Segment.counter
		Segment.counter += 0.2
		
		If parent
			Self.parent = parent
			parent.children.AddLast(Self)
		Else
			Segment.root.AddLast(Self)
		Endif
	End
	
	Method Render()
		SetColor(r,g,b)
		DrawRect(x,y,30,10)
		
		PushMatrix()
		Translate(x+30,y)
		Rotate(Segment.rotation*Self.multiplier)
		For Local segment := Eachin children
			segment.Render()
		Next
		PopMatrix()
	End
End


