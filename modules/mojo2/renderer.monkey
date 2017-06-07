
Import graphics

Private

Import math3d

Class LayerData
	Field matrix:=Mat4New()
	Field invMatrix:=Mat4New()
	Field drawList:DrawList
End

Global lvector:Float[4]
Global tvector:Float[4]
Global pvector:Float[4]
Global tmatrix:Float[16]
Global tmatrix2:Float[16]

Public

Interface ILight

	Method LightMatrix:Float[]()
	Method LightType:Int()
	Method LightColor:Float[]()
	Method LightRange:Float()
	Method LightImage:Image()
	
End

Interface ILayer

	Method LayerMatrix:Float[]()
	Method LayerFogColor:Float[]()
	Method LayerLightMaskImage:Image()
	Method EnumLayerLights:Void( lights:Stack<ILight> )
	Method OnRenderLayer:Void( drawLists:Stack<DrawList> )
	
End

Class Renderer

	Method New()
		_canvas=New Canvas()
		#If TARGET="html5"
			'Handle pyschotic Firefox behaviour...
			_fudgeList=New DrawList
			_fudgeList.DrawRect( 0,0,0,0 )
		#Endif
		Mat4Ortho( 0,_canvas.Width,0,_canvas.Height,-1,1,_projectionMatrix )
	End
	
	Method SetRenderTarget:Void( image:Image )
		_image=image
	End
	
	Method SetViewport:Void( x:Int,y:Int,width:Int,height:Int )
		_viewport[0]=x
		_viewport[1]=y
		_viewport[2]=width
		_viewport[3]=height
	End
	
	Method SetClearMode:Void( clearMode:Int )
		_clearMode=clearMode
	End
	
	Method SetClearColor:Void( clearColor:Float[] )
		_clearColor=clearColor
	End
	
	Method SetAmbientLight:Void( ambientLight:Float[] )
		_ambientLight=ambientLight
	End
	
	Method SetCameraMatrix:Void( cameraMatrix:Float[] )
		_cameraMatrix=cameraMatrix
	End
	
	Method SetProjectionMatrix:Void( projectionMatrix:Float[] )
		_projectionMatrix=projectionMatrix
	End
	
	Method Layers:Stack<ILayer>() Property
		Return _layers
	End
	
	Method Render:Void()
	
		Local vwidth:=_viewport[2],vheight:=_viewport[3]
		
		Local twidth:=vwidth/1,theight:=vheight/1
		
		If Not _timage Or _timage.Width<>twidth Or _timage.Height<>theight
			If _timage _timage.Discard()
			_timage=New Image( twidth,theight,0,0 )
		End
		
		If Not _timage2 Or _timage2.Width<>twidth Or _timage2.Height<>theight
			If _timage2 _timage2.Discard()
			_timage2=New Image( twidth,theight,0,0 )
		End
		
		Mat4Inverse _cameraMatrix,_viewMatrix
		
		Local invProj:=False

		'Clear!		
		_canvas.SetRenderTarget _image	
		_canvas.SetViewport _viewport[0],_viewport[1],_viewport[2],_viewport[3]
		Select _clearMode
		Case 1
			_canvas.Clear _clearColor[0],_clearColor[1],_clearColor[2],_clearColor[3]
		End
		
		For Local layerId:=0 Until _layers.Length
		
			Local layer:=_layers.Get( layerId )
			Local fog:=layer.LayerFogColor
			
			Local layerMatrix:=layer.LayerMatrix
			Mat4Inverse layerMatrix,_invLayerMatrix
			
			_drawLists.Clear
			layer.OnRenderLayer( _drawLists )
			
			Local lights:=New Stack<ILight>
			layer.EnumLayerLights( lights )
			
			If Not lights.Length
			
				For Local i:=0 Until 4
					_canvas.SetLightType i,0
				Next
			
				_canvas.SetRenderTarget _image
				_canvas.SetShadowMap _timage
				_canvas.SetViewport _viewport[0],_viewport[1],_viewport[2],_viewport[3]
				_canvas.SetProjectionMatrix _projectionMatrix
				_canvas.SetViewMatrix _viewMatrix
				_canvas.SetModelMatrix layerMatrix
				_canvas.SetAmbientLight _ambientLight[0],_ambientLight[1],_ambientLight[2],1
				_canvas.SetFogColor fog[0],fog[1],fog[2],fog[3]
				
				_canvas.SetColor 1,1,1,1
				For Local i:=0 Until _drawLists.Length
					_canvas.RenderDrawList _drawLists.Get( i )
				End
				_canvas.Flush
				
				Continue
				
			Endif
			
			Local light0:=0
			
			Repeat
			
				Local numLights:=Min(lights.Length-light0,4)
				
				'Shadows
				'		
				_canvas.SetRenderTarget _timage
				_canvas.SetShadowMap Null
				_canvas.SetViewport 0,0,twidth,theight
				_canvas.SetProjectionMatrix _projectionMatrix
				_canvas.SetViewMatrix _viewMatrix
				_canvas.SetModelMatrix layerMatrix
				_canvas.SetAmbientLight 0,0,0,0
				_canvas.SetFogColor 0,0,0,0
				
				_canvas.Clear 1,1,1,1
				_canvas.SetBlendMode 0
				_canvas.SetColor 0,0,0,0

				_canvas.SetDefaultMaterial Shader.ShadowShader().DefaultMaterial
				
				For Local i:=0 Until numLights
				
					Local light:=lights.Get(light0+i)
					
					Local matrix:=light.LightMatrix
					
					Vec4Copy matrix,lvector,12,0
					Mat4Transform _invLayerMatrix,lvector,tvector
					Local lightx:=tvector[0],lighty:=tvector[1]
					
					_canvas.SetColorMask i=0,i=1,i=2,i=3
					
					Local image:=light.LightImage()
					If image
						_canvas.Clear 0,0,0,0
						_canvas.PushMatrix
						_canvas.SetMatrix matrix[0],matrix[1],matrix[4],matrix[5],lightx,lighty
						_canvas.DrawImage image
						_canvas.PopMatrix
					Endif
		
					For Local j:=0 Until _drawLists.Length
						_canvas.DrawShadows lightx,lighty,_drawLists.Get( j )
					Next
				
				Next
				_canvas.SetDefaultMaterial Shader.FastShader().DefaultMaterial
				_canvas.SetColorMask True,True,True,True
				_canvas.Flush
				
				'LightMask
				'
				Local lightMask:=layer.LayerLightMaskImage()
				If lightMask
				
					If Not invProj
						Mat4Inverse _projectionMatrix,_invProjMatrix
						Mat4Project( _invProjMatrix,[-1.0,-1.0,-1.0,1.0],_ptl )
						Mat4Project( _invProjMatrix,[ 1.0, 1.0,-1.0,1.0],_pbr )
					endif
					
					Local fwidth:=(_pbr[0]-_ptl[0])
					Local fheight:=(_pbr[1]-_ptl[1])
					
					If _projectionMatrix[15]=0
						Local scz:=(layerMatrix[14]-_cameraMatrix[14])/_ptl[2]
						fwidth*=scz
						fheight*=scz
					Endif
				
					_canvas.SetProjection2d 0,fwidth,0,fheight					
					_canvas.SetViewMatrix Mat4Identity
					_canvas.SetModelMatrix Mat4Identity
					
					'test...
					'_canvas.SetBlendMode 0
					'_canvas.SetColor 1,1,1,1
					'_canvas.DrawRect 0,0,fwidth,fheight
					
					_canvas.SetBlendMode 4
					
					Local w:Float=lightMask.Width
					Local h:Float=lightMask.Height
					Local x:=-w
					While x<fwidth+w
						Local y:=-h
						While y<fheight+h
							_canvas.DrawImage lightMask,x,y
							y+=h
						Wend
						x+=w
					Wend
					
					_canvas.Flush

				Endif
				
				'Enable lights
				'
				For Local i:=0 Until numLights
				
					Local light:=lights.Get(light0+i)
					
					Local c:=light.LightColor
					Local m:=light.LightMatrix
					
					_canvas.SetLightType i,1
					_canvas.SetLightColor i,c[0],c[1],c[2],c[3]
					_canvas.SetLightPosition i,m[12],m[13],m[14]
					_canvas.SetLightRange i,light.LightRange
				Next
				For Local i:=numLights Until 4
					_canvas.SetLightType i,0
				Next
				
				If light0=0	'first pass?
				
					'render lights+ambient to output
					'
					_canvas.SetRenderTarget _image
					_canvas.SetShadowMap _timage
					_canvas.SetViewport _viewport[0],_viewport[1],_viewport[2],_viewport[3]
					_canvas.SetProjectionMatrix _projectionMatrix
					_canvas.SetViewMatrix _viewMatrix
					_canvas.SetModelMatrix layerMatrix
					_canvas.SetAmbientLight _ambientLight[0],_ambientLight[1],_ambientLight[2],1
					_canvas.SetFogColor fog[0],fog[1],fog[2],fog[3]
					
					_canvas.SetColor 1,1,1,1
					For Local i:=0 Until _drawLists.Length
						_canvas.RenderDrawList _drawLists.Get( i )
					End
					#If TARGET="html5"
						_canvas.RenderDrawList _fudgeList
					#Endif
					_canvas.Flush
					
				Else
				
					'render lights only
					'
					_canvas.SetRenderTarget _timage2
					_canvas.SetShadowMap _timage
					_canvas.SetViewport 0,0,twidth,theight
					_canvas.SetProjectionMatrix _projectionMatrix
					_canvas.SetViewMatrix _viewMatrix
					_canvas.SetModelMatrix layerMatrix
					_canvas.SetAmbientLight 0,0,0,0
					_canvas.SetFogColor 0,0,0,fog[3]
					
					_canvas.Clear 0,0,0,1
					_canvas.SetColor 1,1,1,1
					For Local i:=0 Until _drawLists.Length
						_canvas.RenderDrawList _drawLists.Get( i )
					End
					_canvas.Flush
					
					'add light to output
					'
					_canvas.SetRenderTarget _image
					_canvas.SetShadowMap Null
					_canvas.SetViewport _viewport[0],_viewport[1],_viewport[2],_viewport[3]
					_canvas.SetProjection2d 0,vwidth,0,vheight
					_canvas.SetViewMatrix Mat4Identity
					_canvas.SetModelMatrix Mat4Identity
					_canvas.SetAmbientLight 0,0,0,1
					_canvas.SetFogColor 0,0,0,0
					
					_canvas.SetBlendMode 2
					_canvas.SetColor 1,1,1,1
					_canvas.DrawImage _timage2
					_canvas.Flush
					
				Endif
				
				light0+=4
			
			Until light0>=lights.Length
			
		Next
	End
	
	Protected
	
	Field _canvas:Canvas
	
#If TARGET="html5"
	Field _fudgeList:DrawList
#Endif
	
	Field _image:Image		'render target
	Field _timage:Image		'tmp lighting texture
	Field _timage2:Image		'another tmp lighting image for >4 lights

	Field _viewport:=[0,0,640,480]
	Field _clearMode:Int=1
	Field _clearColor:=[0.0,0.0,0.0,1.0]
	Field _ambientLight:=[1.0,1.0,1.0,1.0]
	Field _projectionMatrix:=Mat4New()
	Field _cameraMatrix:=Mat4New()
	Field _viewMatrix:=Mat4New()
	
	Field _layers:=New Stack<ILayer>
	
	Field _invLayerMatrix:Float[16]
	Field _drawLists:=New Stack<DrawList>
	
	Field _invProjMatrix:Float[16]
	Field _ptl:Float[4]
	Field _pbr:Float[4]
		
End
