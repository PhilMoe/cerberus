

'Simple drawlist rendering demo.

Import mojo2

Class MyApp Extends App

	Field canvas:Canvas
	
	Field vertices:Float[]
	
	Field indices:Int[]
	
	Method OnCreate()

		canvas=New Canvas
		
		vertices=New Float[4*2*100]
		
		Local sz:=20.0,p:=0

		For Local i:=0 Until 100
		
			Local x:=Rnd(DeviceWidth)-sz/2-DeviceWidth/2
			Local y:=Rnd(DeviceHeight)-sz/2-DeviceHeight/2
			
			vertices[p+0]=x
			vertices[p+1]=y
			
			vertices[p+2]=x+sz
			vertices[p+3]=y
			
			vertices[p+4]=x+sz
			vertices[p+5]=y+sz
			
			vertices[p+6]=x
			vertices[p+7]=y+sz
			
			p+=8
		Next

		'quick test of indices...
		indices=New Int[400]
		For Local i:=0 Until 400
			indices[i]=i
		Next
	End
	
	Method OnRender()
	
		canvas.Clear 0,0,1
		
		canvas.SetColor Sin( Millisecs*.01 )*.5+.5,Cos( Millisecs*.03 )*.5+.5,Sin( Millisecs*.05 )*.5+.5
		
		canvas.PushMatrix
		canvas.Translate MouseX,MouseY
		
'		canvas.DrawPrimitives 4,100,vertices
		canvas.DrawIndexedPrimitives 4,100,vertices,indices	'should draw same thing...
		
		canvas.PopMatrix
	
		canvas.Flush
	End

End

Function Main()
	New MyApp
End
