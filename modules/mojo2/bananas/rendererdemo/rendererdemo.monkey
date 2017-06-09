
'very simple demo of using Renderer for >4 lights + shadows.

'#ANDROID_SCREEN_ORIENTATION="landscape"

Import mojo2

Const NUM_LIGHTS:=5

'create an orthographics projection matrix
Function Mat4Ortho:Float[]( left:Float,right:Float,bottom:Float,top:Float,znear:Float,zfar:Float )

	Local w:=right-left,h:=top-bottom,d:=zfar-znear
	
	Return [ 2.0/w,0,0,0, 0,2.0/h,0,0, 0,0,2.0/d,0, -(right+left)/w,-(top+bottom)/h,-(zfar+znear)/d,1 ]
End

Class MyLight Implements ILight

	'note: x,y,z,w go in last 4 components of matrix...
	Field matrix:=[1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,-100.0,1.0]
	Field color:=[0.2,0.2,0.2,1.0]
	Field range:=400.0

	'implement ILight interface...
	'
	Method LightMatrix:Float[]()
		Return matrix
	End
	
	Method LightType:Int()
		Return 1
	End
	
	Method LightColor:Float[]()
		Return color
	End
	
	Method LightRange:Float()
		Return range
	End
	
	Method LightImage:Image()
		Return Null
	End

End

Class MyLayer Extends DrawList Implements ILayer

	Field lights:=New Stack<MyLight>
	Field layerMatrix:=[1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0]
	Field layerFogColor:=[0.0,0.0,0.0,0.0]

	'implement ILayer interface...
	'
	Method LayerMatrix:Float[]()
		Return layerMatrix
	End
	
	Method LayerFogColor:Float[]()
		Return layerFogColor
	End
	
	Method LayerLightMaskImage:Image()
		Return Null
	End
	
	Method EnumLayerLights:Void( lights:Stack<ILight> )
		For Local light:=Eachin Self.lights
			lights.Push light
		Next
	End

	Method OnRenderLayer:Void( drawLists:Stack<DrawList> )
		drawLists.Push Self
	End

End

Class MyApp Extends App

	Field tile:Image
	Field shadowCaster:ShadowCaster
	Field renderer:Renderer
	Field layer0:MyLayer
	Field rimage:Image
	Field weird:=New DrawList
	
	Method OnCreate()
	
		'create renderer
		renderer=New Renderer
		renderer.SetViewport( 0,0,DeviceWidth,DeviceHeight )
		renderer.SetProjectionMatrix( Mat4Ortho( 0,640,0,480,-1,1 ) )
		renderer.SetAmbientLight( [0.1,0.1,0.1,1.0] )
		
		'load some gfx
		tile=Image.Load( "t3.png",0,0 )
		
		'create layer 0
		layer0=New MyLayer
		
		'add some lights to layer
		For Local i:=0 Until NUM_LIGHTS
			layer0.lights.Push New MyLight
		Next

		For Local x:=0 Until 640 Step 128
			For Local y:=0 Until 480 Step 128	
				layer0.DrawImage tile,x,y
			Next
		Next
		
		'create simple rect shadow caster
		shadowCaster=New ShadowCaster
		shadowCaster.SetVertices( [0.0,0.0, 32.0,0.0, 32.0,32.0, 0.0,32.0] )
		
		'draw some shadow casters
		For Local x:=100 Until 640 Step 220
		
			For Local y:=60 Until 480 Step 180
			
				layer0.SetColor 1,1,0
				layer0.DrawRect x-16,y-16,32,32
				layer0.SetColor 1,1,1
				
				layer0.AddShadowCaster shadowCaster,x-16,y-16
			Next
		Next

		'add layer to renderer		
		renderer.Layers.Push layer0
	End
	
	Method OnRender()
	
		renderer.SetViewport( 0,0,DeviceWidth,DeviceHeight )
	
		'move lights around a bit
		For Local i:=0 Until NUM_LIGHTS
			Local light:=layer0.lights.Get(i)
			Local radius:=120.0
			Local an:=(i*360.0/NUM_LIGHTS)+(Millisecs/50.0)
			light.matrix[12]=Cos( an )*radius+320
			light.matrix[13]=Sin( an )*radius+240
		Next

		'render scene
		renderer.Render
		
	End
End

Function Main()

	New MyApp

End
