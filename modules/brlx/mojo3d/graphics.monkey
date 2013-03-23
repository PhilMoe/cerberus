
Extern

Class BBVertexBuffer

	Method _New:Bool( length:Int,format:Int[],flags:Int )
	
	Method SetData:Void( first:Int,count:Int,data:BBDataBuffer,offset:Int )
	Method GetData:Void( first:Int,count:Int,data:BBDataBuffer,offset:Int )
	
End

Class BBIndexBuffer

	Method _New:Bool( length:Int,format:Int,flags:Int )
	
	Method SetData:Void( first:Int,count:Int,data:BBDataBuffer,offset:Int )
	Method GetData:Void( first:Int,count:Int,data:BBDataBuffer,offset:Int )
	
End

Class BBTexture

	Method _New2d:Bool( width:Int,height:Int,format:Int,flags:Int )
	Method _New3d:Bool( width:Int,height:Int,depth:Int,format:Int,flags:Int )
	Method _NewCube:Bool( width:Int,height:Int,format:Int,flags:Int )

	Method SetData2d:Void( x:Int,y:Int,width:Int,height:Int,data:BBDataBuffer,offset:Int )
	Method GetData2d:Void( x:Int,y:Int,width:Int,height:Int,data:BBDataBuffer,offset:Int )

	Method SetData3d:Void( x:Int,y:Int,z:Int,width:Int,height:Int,depth:Int,data:BBDataBuffer,offset:Int )
	Method GetData3d:Void( x:Int,y:Int,z:Int,width:Int,height:Int,depth:Int,data:BBDataBuffer,offset:Int )

	Method SetDataCube:Void( x:Int,y:Int,face:Int,width:Int,height:Int,data:BBDataBuffer,offset:Int )
	Method GetDataCube:Void( x:Int,y:Int,face:Int,width:Int,height:Int,data:BBDataBuffer,offset:Int )
	
End

Class BBShaderProgram

	Method _New:Bool( source:String )
	
End

Class BBShaderParams

	Method _New:Bool()
	
	Method SetParam:Void( name:String,value:Float[] )
	
	Method SetParam:Void( name:String,value:Texture )
	
End

Class BBGraphicsDevice

End

Public

Class VertexFormat
	Const Position3f:=1
	Const Normal3f:=2
	Const Tangent4f:=3
	Const TexCoords2f:=4
End

Const IndexFormat
	Const Int8:=1
	Const Int16:=2
	Const Int32:=3
End
	
Class TextureFormat
	Const Color:=1
	Const Alpha:=2
	Const ColorAlpha:=3
	Const Depth:=4
End

Class BufferFlags
	Const Static:=0
	Const Dynamic:=1
End

Class TextureFlags Extends BufferFlags
	Const RenderTarget:=2
End

Class CubeFace
	Const Front:=1
	Const Back:=2
	Const Left:=3
	Const Right:=4
	Const Top:=5
	Const Bottom:=6
End

Public

Class GraphicsContext

End


#rem

Class CmpFunc
	Const Fail:=0
	Const Less:=1
	Const Greater:=2
	Const NotEqual:=3
	Const Equal:=4
	Const LessEqual:=5
	Const GreaterEqual:=6
	Const Pass:=7
End

Class BlendFactor
	Const Zero:=0
	Const One:=1
	Const SrcAlpha:=2
	Const DstAlpha:=3
	Const OneMinusSrcAlpha:=4
	Const OneMinusDstAlpha:=5
End

Class CullMode
	Const None:=0
	Const Back:=1
	Const Front:=2
End

Class DepthMode
	Const None:=0
	Const Read:=1
	Const Write:=2
	Const Update:=3
End

Class BlendEquation
	Const Add:=1
	Const Subtract:=2
	Const ReverseSubtract:=3
	Const Min:=4
	Const Max:=5
End

Class CubeFace
	Const Front:=1
	Const Back:=2
	Const Left:=3
	Const Right:=4
	Const Top:=5
	Const Bottom:=6
End

Class TextureFormat
	Const Color:=1
	Const Alpha:=2
	Const ColorAlpha:=3
	Const Luminance:=4
	Const LuminanceAlpha:=5
	Const Depth:=6
End

Class TextureFlags
	Const AutoMipmap:=1
	Const RenderTarget:=2
End

Class AttribFormat
	Const Position3f:=1
	Const Normal3f:=2
	Const Tangent4f:=3
	Const TexCoords2f:=4
End

Class VertexBuffer

	Method New( length:Int,format:Int[] )
	End
	
	Method WriteData:Void( first:Int,count:Int,data:DataBuffer,offset:Int,pitch:Int )
	End
	
	Method ReadData:Void( first:Int,count:Int,data:DataBuffer,offset:Int,pitch:Int )
	End
End

Class IndexBuffer

	Method New( length:Int,format:Int )
	
	Method WriteData:Void( first:Int,count:Int,data:DataBuffer,offset:Int,pitch:Int )
	
	Method ReadData:Void( first:Int,count:Int,data:DataBuffer,offset:Int,pitch:Int )
	
End

Class Texture
End

Class Texture2d Extends Texture

	Method New( width:Int,height:Int,format:Int,flags:Int )
	
	Method WriteData:Void( x:Int,y:Int,width:Int,height:Int,data:DataBuffer,offset:Int,pitch:Int )

	Method ReadData:Void( x:Int,y:Int,width:Int,height:Int,data:DataBuffer,offset:Int,pitch:Int )
	
End

Class Texture3d Extends Texture

	Method New( width:Int,height:Int,depth:Int,format:Int,flags:Int )

	Method WriteData:Void( x:Int,y:Int,z:Int,width:Int,height:Int,data:DataBuffer,offset:Int,pitch:Int )

	Method ReadData:Void( x:Int,y:Int,z:Int,width:Int,height:Int,data:DataBuffer,offset:Int,pitch:Int )
	
End

Class TextureCube Extends Texture

	Method New( width:Int,height:Int,format:Int,flags:Int )

	Method WriteData:Void( x:Int,y:Int,face:Int,width:Int,height:Int,data:DataBuffer,offset:Int,pitch:Int )

	Method ReadData:Void( x:Int,y:Int,face:Int,width:Int,height:Int,data:DataBuffer,offset:Int,pitch:Int )
	
	
End

Class ShaderProgram

	Method New( source:String )
	
End

Class ShaderParam

End

Class VectorParam Extends ShaderParam

	Method New( name:String,value:Float[] )
	
	Method SetValue:Void( value:Float[] )
	
	Method GetValue:Float[]()
End

Class TextureParam Extends ShaderParam

	Method New( value:Texture )
End

Class GraphicsContext

	Method SetViewport:Void( x:Int,y:Int,width:Int,height:Int )
	End

	Method SetCullMode:Void( mode:Int )
	End

	Method SetDepthMode:Void( mode:Int )
	End

	Method SetDepthFunc:Void( func:Int )
	End

	Method SetBlendEquation:Void( mode:Int )
	End

	Method SetBlendFunc:Void( srcFactor:Int,dstFactor:Int )
	End
	
	Method SetShaderProgram:Void( program:ShaderProgram )
	End
	
	Method SetShaderParams:Void( name:String,params:ShaderParam[] )
	End
	
	Method SetVertexBuffer:Void( vbuffer:VertexBuffer )
	End
	
	Method SetIndexBuffer:Void( ibuffer:IndexBuffer )
	End
	
	Method Render:Void( what:Int,first:Int,count:Int )
	End
	
End
