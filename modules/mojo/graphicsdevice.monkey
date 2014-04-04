
#If MOJO_VERSION_X
#Error "Mojo version error"
#Endif

Private

Import driver

Extern 

Class GraphicsDevice="gxtkGraphics"

	'Can be used outside of OnRender
	Method Width()
	Method Height()
	Method LoadSurface:Surface( path$ )
	Method CreateSurface:Surface( width,height )
	Method WritePixels2( surface:Surface,pixels[],x,y,width,height,offset,pitch )

	'Begin/end rendering	
	Method BeginRender:Int()	'0=gl, 1=mojo, 2=loading
	Method EndRender:Void()
	Method DiscardGraphics:Void()
	
	'Render only ops - can only be used during OnRender
	Method Cls( r#,g#,b# )
	Method SetAlpha( alpha# )
	Method SetColor( r#,g#,b# )
	Method SetMatrix( ix#,iy#,jx#,jy#,tx#,ty# )
	Method SetScissor( x,y,width,height )
	Method SetBlend( blend )
	
	Method DrawPoint( x#,y# )
	Method DrawRect( x#,y#,w#,h# )
	Method DrawLine( x1#,y1#,x2#,y2# )
	Method DrawOval( x#,y#,w#,h# )
	Method DrawPoly( verts#[] )
	Method DrawPoly2( verts#[],surface:Surface,srcx,srcy )
	Method DrawSurface( surface:Surface,x#,y# )
	Method DrawSurface2( surface:Surface,x#,y#,srcx,srcy,srcw,srch )
	
	Method ReadPixels( pixels[],x,y,width,height,offset,pitch )
	
	'INTERNAL - subject to change etc.
	Method LoadSurface__UNSAFE__:bool( surface:Surface,path$ )

End

Class Surface="gxtkSurface"

	Method Discard()

	Method Width() Property
	Method Height() Property
	Method Loaded() Property

	'INTERNAL - subject to change etc.
	Method OnUnsafeLoadComplete:Void()

End
