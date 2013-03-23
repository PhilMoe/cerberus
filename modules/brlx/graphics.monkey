
Class TextureFormat
	Const Alpha=1
	Const Color=2
	Const ColorAlpha=2
	Const Normal=4
	Const Depth=3
End

Class GraphicsBufferFlags
	Const Static=1
	Const Stream=2
	Const Dynamic=3
End

Class Texture2DFlags Extends GraphicsBufferFlags
	Const RenderTarget=4
End

Class Texture3DFlags Extends GraphicsBufferFlags
End

Class TextureCubeFlags Extends GraphicsBufferFlags
End

Class VertexBufferFlags Extends GraphicsBufferFlags
End

Class IndexBufferFlags Extends GraphicsBufferFlags
End

Class RenderOp
	Const Points=1
	Const Lines=2
	Const Triangles=3
End

Class GraphicsBuffer
End

Class Texture Extends GraphicsBuffer
End

Class Texture2D Extends Texture

	Method SetData:Void( x:Int,y:Int,width:Int,height:Int,data:DataBuffer,dataOffset:Int,dataPitch:Int )

	Method GetData:Void( x:Int,y:Int,width:Int,height:Int,data:DataBuffer,dataOfffset:Int,dataPitch:Int )
	
End

Class VertexBuffer

	Method SetData:Void( first:Int,count:Int,data:DataBuffer,dataOffset:Int )

	Method GetData:Void( first:Int,count:Int,data:DataBuffer,dataOffset:Int )

End

Class Graphics3d

	Method CreateShader:Shader( source:String )
		
	Method CreateTexture2D:Texture2D( width:Int,height:Int,format:Int,flags:Int )
	
	Method CreateTexture3D:Texture3D( width:Int,height:Int,depth:Int,format:Int,flags:Int )
	
	Method CreateTextureCube:TextureCube( width:Int,height:Int,format:Int,flags:Int )
	
	Method CreateVertexBuffer:VertexBuffer( length:Int,format:Int[],flags:Int )
	
	Method CreateIndexBuffer:IndexBuffer( lemgth:Int,format:Int,flags:Int )
	
	Method SetViewport:Void( x:Int,y:Int,width:Int,height:Int )

	Method SetDepthMode:Void( mode:Int )
	
	Method SetColorMode:Void( mode:Int )
	
	Method SetBlendMode:Void( mode:Int )
	
	Method SetStencilMode:Void( mode:Int )

	Method SetShaderParam:Void( id:String,floats:Float[] )
	
	Method SetShaderParam:Void( id:String,texture:Texture )
	
	Method SetShader:Void( shader:Shader )
	
	Method SetRenderTarget:Void( index:Int,texture:Texture )
	
	Method SetVertexBuffer:Void( offset:Int,buffer:VertexBuffer )
	
	Method SetIndexBuffer:Void( buffer:IndexBuffer )

	Method Clear:Void( r:Float,g:Float,b:Float,a:Float,depth:Float,stencil:Int,clearColor:Bool,clearDepth:Bool,clearStencil:Bool )
	
	Method Render:Void( op:Int,offset:Int,count:Int )
	
End
