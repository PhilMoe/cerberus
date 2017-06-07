

Import mojo2

Import mojo2.glutil

Class MyApp Extends App

	Field canvas:Canvas
	Field image:Image
	
	Method OnCreate()
	
		'generate color texture
		Local colortex:=New Texture( 256,256,4,Texture.ClampST|Texture.RenderTarget )
		Local rcanvas:=New Canvas( colortex )
		rcanvas.Clear( 1,1,1 )
		rcanvas.Flush

		'generate normal texture		
		Local normtex:=New Texture( 256,256,4,Texture.ClampST|Texture.RenderTarget )
		rcanvas.SetRenderTarget( normtex )
		rcanvas.Clear( .5,.5,1.0,0.0 )
		For Local x:=0 Until 256 'Step 32
			For Local y:=0 Until 256 'Step 32
				
				Local dx:=x-127.5
				Local dy:=y-127.5
				Local dz:=127.5*127.5-dx*dx-dy*dy
				
				If dz<=0 Continue
				
				dz=Sqrt( dz )
				
				Local r:=(dx+127.5)/255.0
				Local g:=(dy+127.5)/-255.0
				Local b:=(dz+127.5)/255.0
				
				rcanvas.SetColor( r,g,b,1 )
				rcanvas.DrawPoint( x,y )

			Next
		Next
		rcanvas.Flush
		
		Local material:=New Material( Shader.BumpShader() )
		material.SetTexture( "ColorTexture",colortex )
		material.SetTexture( "NormalTexture",normtex )
		material.SetVector( "AmbientColor",[0.0,0.0,0.0,1.0] )
		
		image=New Image( material,.5,.5 )
	
		canvas=New Canvas
		canvas.SetAmbientLight .2,.2,.2
	End
	
	Field rot:Float
	
	Method OnRender()
	
		canvas.Clear 0,0,0
		
		'Set light 0
		canvas.SetLightType 0,1
		canvas.SetLightColor 0,.3,.3,.3
		canvas.SetLightPosition 0,MouseX,MouseY,-100
		canvas.SetLightRange 0,400
		
		rot+=1
		
		canvas.DrawImage image,DeviceWidth/2,DeviceHeight/2,rot,.5,.5
		
		canvas.Flush
		
	End

End

Function Main()

	New MyApp

End
