
Strict

Import mojo2

Class MyApp Extends App

	Field canvas:Canvas	
	Field drawing:List<TriPrims>
	Field rotstep:Float
	Field wiggle:Float
	Field rot:Float
 
	Method OnCreate:Int()
		canvas=New Canvas
		drawing = MakeVectorDrawing()
		Return 0
	End
	
	Method OnUpdate:Int()
	
		 If KeyHit(KEY_W) 
		 	wiggle = 0.5

		 Endif
		 
		 If MouseDown(0)
		 	For Local pt:= Eachin drawing 		 
	 			pt.Pull(MouseX(),MouseY(),0.0001) 		
	 		Next
		 Endif
		 
		 If MouseDown(1)
		 	For Local pt:= Eachin drawing 		 
	 			pt.Pull(MouseX(),MouseY(),-0.0001) 		
	 		Next
		 Endif
	
		If KeyHit(KEY_SPACE)
			wiggle = 0
			rot = 0
			For Local pt:= Eachin drawing 		 
	 			pt.ResetMorph(1)	
	 		Next
			 
		 
			rotstep = 0
		Endif 
	
		Return 0
	End Method
	
	
	Method OnRender:Int()
	
		canvas.ResetMatrix()
	
		canvas.Clear .5,.7,1
		 	
		canvas.SetColor 0.5,0.5,0.5
		canvas.DrawText "Mouse Click with left/right mouse button",10,10
		canvas.DrawText "Press [W] to wiggle",10,30
		canvas.DrawText "Press [Space] to reset",10,50
		 
		rot = 0
		 	
	 	For Local pt:= Eachin drawing
	 		canvas.Rotate rot
	 		pt.Draw(canvas)
	 		canvas.Rotate -rot
	 		rot +=rotstep
	 		
	 	Next
	 	
		canvas.Flush
		rotstep +=Cos(Float(Millisecs() Mod 360))*wiggle
	 
		wiggle *=0.95
		
		rotstep *=0.99
		
		For Local pt:= Eachin drawing 		 
	 			pt.ResetMorph(0.01)	
	 		Next
		
		Return 0
	End
End


Function MakeVectorDrawing:List<TriPrims>()

	Local data:=LoadString( "data.txt" )

	Local primslist:= New List<TriPrims>
	
	
		Local lines:String[] = data.Split("*")
	
	Local tp:TriPrims
	
	For Local line:String = Eachin lines
		Local parts:String[] = line.Split(";")
	
		 
		 select parts[0]
		 
		 	Case "color"
				  tp = New TriPrims
				
				Local vals:String[] = parts[1].Split(",")
				tp.r = Float(vals[0])
				tp.g = Float(vals[1])
				tp.b = Float(vals[2])
			 	primslist.AddLast(tp)
			 	
			 Case "vertices"	
			 	Local vals:String[] = parts[1].Split(",")
			 	
			 	tp.vertices = New Float[vals.Length]
			 	For Local i:Int = 0  Until vals.Length
			 		tp.vertices[i] = Float(vals[i])*0.01
			 	Next
			 			
				 
			 Case "indexes"				
				Local vals:String[] = parts[1].Split(",")
			 	tp.indexes = New Int[vals.Length]
			 	For Local i:Int = 0  Until vals.Length
			 		tp.indexes[i] = Int(vals[i])
			 	Next			
		 
		 End Select
		 
		 Next
		 
		 
		 Return primslist
		 
End Function


Function Main:Int()
	New MyApp
	Return 0
End

Class TriPrims
	Field r:Float,g:Float,b:Float,a:Float = 1
	Field vertices:Float[]
	Field morphedvertices:Float[]
	Field indexes:Int[]
	
	Method ResetMorph:Void(factor:Float = 1.0)
	
		If factor>= 1
			Local invfactor:Float = 1.0 - factor
			For Local i:Int = 0 Until vertices.Length
				morphedvertices[i] =  vertices[i]  
			Next
			Return 
		endif 
	
	
		Local invfactor:Float = 1.0 - factor
		For Local i:Int = 0 Until vertices.Length
			Local newval:Float =  morphedvertices[i] *invfactor +  vertices[i] * factor
		
		Next
	End Method
	
	
	
	Method Draw:void(canvas:Canvas)
		If morphedvertices.Length<>vertices.Length
			morphedvertices= New  Float[vertices.Length]
			ResetMorph()
	 	Endif
	
		canvas.SetColor r,g,b,a
		canvas.DrawIndexedPrimitives(3,indexes.Length/3,morphedvertices,indexes)
	End Method
	
	Method Pull:Void(x:Float,y:Float,factor:float)
		
		For Local i:Int = 0 Until vertices.Length Step 2
		
			Local vx:Float = (morphedvertices[i]-x)
			Local vy:Float = (morphedvertices[i+1]-y) 
		
			Local dist:Float = Sqrt(vx*vx+vy*vy)
		
	 		dist = 200-dist

			Local newx:Float = morphedvertices[i]  + vx  * dist  * factor 
			Local newy:Float = morphedvertices[i+1]  + vy *  dist  * factor 
 
			morphedvertices[i] = newx
			morphedvertices[i+1] = newy
 
		Next
	End Method
	
End Class
