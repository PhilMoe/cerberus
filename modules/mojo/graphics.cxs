' Module mojo.graphics
'
' Copyright 2011 Mark Sibly, all rights reserved.
' No warranty implied; use at your own risk.

#If MOJO_VERSION_X
Import mojox.graphics
#Else

Private

Import "data/mojo_font.png"

Import graphicsdevice
Import data
Import app

Class Frame

	Field x,y
	
	Method New( x,y )
		Self.x=x
		Self.y=y
	End

End

Class GraphicsContext

	Field color_r#,color_g#,color_b#
	Field alpha#
	Field blend
	Field ix#=1,iy#,jx#,jy#=1,tx#,ty#,tformed,matDirty
	Field scissor_x#,scissor_y#,scissor_width#,scissor_height#
	Field matrixStack:=New Float[6*32],matrixSp
	Field font:Image,firstChar,defaultFont:Image

	Method Validate()
		If matDirty
			renderDevice.SetMatrix context.ix,context.iy,context.jx,context.jy,context.tx,context.ty
			matDirty=0
		Endif
	End
End

Global device:GraphicsDevice
Global renderDevice:GraphicsDevice

Global context:GraphicsContext=New GraphicsContext

Function DebugRenderDevice()
	If Not renderDevice Error "Rendering operations can only be performed inside OnRender"
End

Public

Const AlphaBlend=0
Const AdditiveBlend=1
Const LightenBlend=1	'deprecated

Function SetGraphicsDevice( dev:GraphicsDevice )
	device=dev
End

Function GetGraphicsDevice:GraphicsDevice()
	Return device
End

Class Image

	Const MidHandle=1
	
	Const XPadding=2
	Const YPadding=4
	Const XYPadding=XPadding|YPadding

	Global DefaultFlags

	Method Width()
		Return width
	End

	Method Height()
		Return height
	End
	
	Method Loaded()
		Return surface.Loaded()
	End
	
	Method Frames()
		Return frames.Length
	End
	
	Method Flags()
		Return flags
	End

	Method HandleX#()
		Return tx
	End
	
	Method HandleY#()
		Return ty
	End
	
	Method GrabImage:Image( x,y,width,height,nframes=1,flags=DefaultFlags )
		If frames.Length<>1 Return
		Return (New Image).Init( surface,x,y,width,height,nframes,flags,Self,frames[0].x,frames[0].y,Self.width,Self.height )
	End
	
	Method SetHandle( tx#,ty# )
		Self.tx=tx
		Self.ty=ty
		Self.flags=Self.flags & ~MidHandle
	End
	
	Method Discard()
		If surface And Not source
			surface.Discard
			surface=Null
		Endif
	End
	
	Method WritePixels( pixels[],x,y,width,height,offset=0,pitch=0,frame=0 )
		If Not pitch pitch=width

#If CONFIG="debug"
		Local w:=Self.width
		If flags & XPadding w+=2
		Local h:=Self.height
		If flags & YPadding h+=2
		If frame<0 Or frame>=frames.Length Error "Invalid frame"
		If x<0 Or y<0 Or x+width>w Or y+height>h Error "Invalid pixel rectangle"
		If offset<0 Or pitch<0 Or offset+(height-1)*pitch+width>pixels.Length Error "Invalid array rectangle"
#End
		device.WritePixels2 surface,pixels,frames[frame].x+x,frames[frame].y+y,width,height,offset,pitch
	End
	
	'***** INTERNAL *****
	
	Method Init:Image( surf:Surface,nframes,iflags )
		If surface Error "Image already initialized"
		surface=surf
			
		width=surface.Width/nframes
		height=surface.Height
		
		frames=New Frame[nframes]
		For Local i=0 Until nframes
			frames[i]=New Frame( i*width,0 )
		Next
		
		ApplyFlags iflags
		Return Self
	End
	
	Method Init:Image( surf:Surface,x,y,iwidth,iheight,nframes,iflags,src:Image,srcx,srcy,srcw,srch )
		If surface Error "Image already initialized"
		surface=surf
		source=src

		width=iwidth
		height=iheight
		
		frames=New Frame[nframes]
		
		Local ix:=x,iy:=y
		
		For Local i=0 Until nframes
			If ix+width>srcw
				ix=0
				iy+=height
			Endif
			If ix+width>srcw Or iy+height>srch
				Error "Image frame outside surface"
			Endif
			frames[i]=New Frame( ix+srcx,iy+srcy )
			ix+=width
		Next
		
		ApplyFlags iflags
		Return Self
	End
	
Private

	Const FullFrame=65536

	Field source:Image
	Field surface:Surface
	Field width,height,flags
	Field frames:Frame[]
	Field tx#,ty#
	
	Method ApplyFlags( iflags )
		flags=iflags
		
		If flags & XPadding
			For Local f:=Eachin frames
				f.x+=1
			Next
			width-=2
		Endif
		
		If flags & YPadding
			For Local f:=Eachin frames
				f.y+=1
			Next
			height-=2
		Endif
		
		If flags & Image.MidHandle
			SetHandle width/2.0,height/2.0
		Endif
		
		If frames.Length=1 And frames[0].x=0 And frames[0].y=0 And width=surface.Width And height=surface.Height
			flags|=FullFrame
		Endif
	End

End	

Function BeginRender()
	renderDevice=device
	context.matrixSp=0
	SetMatrix 1,0,0,1,0,0
	SetColor 255,255,255
	SetAlpha 1
	SetBlend 0
	SetScissor 0,0,DeviceWidth,DeviceHeight
End

Function EndRender()
	renderDevice=Null
End

Function LoadImage:Image( path$,frameCount=1,flags=Image.DefaultFlags )
	Local surf:=device.LoadSurface( FixDataPath(path) )
	If surf Return (New Image).Init( surf,frameCount,flags )
End

Function LoadImage:Image( path$,frameWidth,frameHeight,frameCount,flags=Image.DefaultFlags )
	Local surf:=device.LoadSurface( FixDataPath(path) )
	If surf Return (New Image).Init( surf,0,0,frameWidth,frameHeight,frameCount,flags,Null,0,0,surf.Width,surf.Height )
End

Function CreateImage:Image( width,height,frameCount=1,flags=Image.DefaultFlags )
	Local surf:=device.CreateSurface( width*frameCount,height )
	If surf Return (New Image).Init( surf,frameCount,flags )
End

Function SetColor( r#,g#,b# )
	context.color_r=r
	context.color_g=g
	context.color_b=b
	renderDevice.SetColor r,g,b
End

Function GetColor#[]()
	Return [context.color_r,context.color_g,context.color_b]
End

Function GetColor( color#[] )
	color[0]=context.color_r
	color[1]=context.color_g
	color[2]=context.color_b
End

Function SetAlpha( alpha# )
	context.alpha=alpha
	renderDevice.SetAlpha alpha
End

Function GetAlpha#()
	Return context.alpha
End

Function SetBlend( blend )
	context.blend=blend
	renderDevice.SetBlend blend
End

Function GetBlend()
	Return context.blend
End

Function SetScissor( x#,y#,width#,height# )
	context.scissor_x=x
	context.scissor_y=y
	context.scissor_width=width
	context.scissor_height=height
	renderDevice.SetScissor x,y,width,height
End

Function GetScissor#[]()
	Return [context.scissor_x,context.scissor_y,context.scissor_width,context.scissor_height]
End

Function GetScissor( scissor#[] )
	scissor[0]=context.scissor_x
	scissor[1]=context.scissor_y
	scissor[2]=context.scissor_width
	scissor[3]=context.scissor_height
End

Function SetMatrix( m#[] )
	SetMatrix m[0],m[1],m[2],m[3],m[4],m[5]
End

Function SetMatrix( ix#,iy#,jx#,jy#,tx#,ty# )
	context.ix=ix
	context.iy=iy
	context.jx=jx
	context.jy=jy
	context.tx=tx
	context.ty=ty
	context.tformed=(ix<>1 Or iy<>0 Or jx<>0 Or jy<>1 Or tx<>0 Or ty<>0)
	context.matDirty=1
End

Function GetMatrix#[]()
	Return [context.ix,context.iy,context.jx,context.jy,context.tx,context.ty]
End

Function GetMatrix( matrix#[] )
	matrix[0]=context.ix;matrix[1]=context.iy;
	matrix[2]=context.jx;matrix[3]=context.jy;
	matrix[4]=context.tx;matrix[5]=context.ty
End

Function PushMatrix()
	Local sp:=context.matrixSp
	If sp=context.matrixStack.Length context.matrixStack=context.matrixStack.Resize( sp*2 )
	context.matrixStack[sp+0]=context.ix
	context.matrixStack[sp+1]=context.iy
	context.matrixStack[sp+2]=context.jx
	context.matrixStack[sp+3]=context.jy
	context.matrixStack[sp+4]=context.tx
	context.matrixStack[sp+5]=context.ty
	context.matrixSp=sp+6
End

Function PopMatrix()
	Local sp=context.matrixSp-6
	SetMatrix context.matrixStack[sp+0],context.matrixStack[sp+1],context.matrixStack[sp+2],context.matrixStack[sp+3],context.matrixStack[sp+4],context.matrixStack[sp+5]
	context.matrixSp=sp
End

Function Transform( m#[] )
	Transform m[0],m[1],m[2],m[3],m[4],m[5]
End

Function Transform( ix#,iy#,jx#,jy#,tx#,ty# )
	Local ix2#=ix*context.ix+iy*context.jx
	Local iy2#=ix*context.iy+iy*context.jy
	Local jx2#=jx*context.ix+jy*context.jx
	Local jy2#=jx*context.iy+jy*context.jy
	Local tx2#=tx*context.ix+ty*context.jx+context.tx
	Local ty2#=tx*context.iy+ty*context.jy+context.ty
	SetMatrix ix2,iy2,jx2,jy2,tx2,ty2
End

Function Translate( x#,y# )
	Transform 1,0,0,1,x,y
End Function

Function Scale( x#,y# )
	Transform x,0,0,y,0,0
End Function

Function Rotate( angle# )
	Transform Cos(angle),-Sin(angle),Sin(angle),Cos(angle),0,0
End Function

Function Cls( r#=0,g#=0,b#=0 )
#If CONFIG="debug"
	DebugRenderDevice
#End
	renderDevice.Cls r,g,b
End

Function DrawPoint( x#,y# )
#If CONFIG="debug"
	DebugRenderDevice
#End
	context.Validate
	renderDevice.DrawPoint x,y
End

Function DrawRect( x#,y#,w#,h# )
#If CONFIG="debug"
	DebugRenderDevice
#End
	context.Validate
	renderDevice.DrawRect x,y,w,h
End

Function DrawLine( x1#,y1#,x2#,y2# )
#If CONFIG="debug"
	DebugRenderDevice
#End
	context.Validate
	renderDevice.DrawLine x1,y1,x2,y2
End

Function DrawOval( x#,y#,w#,h# )
#If CONFIG="debug"
	DebugRenderDevice
#End
	context.Validate
	renderDevice.DrawOval x,y,w,h
End

Function DrawCircle( x#,y#,r# )
#If CONFIG="debug"
	DebugRenderDevice
#End
	context.Validate
	renderDevice.DrawOval x-r,y-r,r*2,r*2
End

Function DrawEllipse( x#,y#,xr#,yr# )
#If CONFIG="debug"
	DebugRenderDevice
#End
	context.Validate
	renderDevice.DrawOval x-xr,y-yr,xr*2,yr*2
End

Function DrawPoly( verts#[] )
#If CONFIG="debug"
	DebugRenderDevice
#End
	context.Validate
	renderDevice.DrawPoly verts
End

Function DrawPoly( verts#[],image:Image,frame:Int=0 )
#If CONFIG="debug"
	DebugRenderDevice
	If frame<0 Or frame>=image.frames.Length Error "Invalid image frame"
#End
	Local f:=image.frames[frame]
	context.Validate
	renderDevice.DrawPoly2 verts,image.surface,f.x,f.y
End

Function DrawImage( image:Image,x#,y#,frame=0 )

#If CONFIG="debug"
	DebugRenderDevice
	If frame<0 Or frame>=image.frames.Length Error "Invalid image frame"
#End

	Local f:Frame=image.frames[frame]

	context.Validate
	
	If image.flags & Image.FullFrame
		renderDevice.DrawSurface image.surface,x-image.tx,y-image.ty
	Else
		renderDevice.DrawSurface2 image.surface,x-image.tx,y-image.ty,f.x,f.y,image.width,image.height
	Endif
End

Function DrawImage( image:Image,x#,y#,rotation#,scaleX#,scaleY#,frame=0 )

#If CONFIG="debug"
	DebugRenderDevice
	If frame<0 Or frame>=image.frames.Length Error "Invalid image frame"
#End

	Local f:Frame=image.frames[frame]

	PushMatrix

	Translate x,y
	Rotate rotation
	Scale scaleX,scaleY

	Translate -image.tx,-image.ty

	context.Validate
	
	If image.flags & Image.FullFrame
		renderDevice.DrawSurface image.surface,0,0
	Else
		renderDevice.DrawSurface2 image.surface,0,0,f.x,f.y,image.width,image.height
	Endif

	PopMatrix
End

Function DrawImageRect( image:Image,x#,y#,srcX,srcY,srcWidth,srcHeight,frame=0 )

#If CONFIG="debug"
	DebugRenderDevice
	If frame<0 Or frame>=image.frames.Length Error "Invalid image frame"
	If srcX<0 Or srcY<0 Or srcX+srcWidth>image.width Or srcY+srcHeight>image.height Error "Invalid image rectangle"
#End

	Local f:Frame=image.frames[frame]

	context.Validate
	
	renderDevice.DrawSurface2 image.surface,-image.tx+x,-image.ty+y,srcX+f.x,srcY+f.y,srcWidth,srcHeight
End

Function DrawImageRect( image:Image,x#,y#,srcX,srcY,srcWidth,srcHeight,rotation#,scaleX#,scaleY#,frame=0 )

#If CONFIG="debug"
	DebugRenderDevice
	If frame<0 Or frame>=image.frames.Length Error "Invalid image frame"
	If srcX<0 Or srcY<0 Or srcX+srcWidth>image.width Or srcY+srcHeight>image.height Error "Invalid image rectangle"
#End

	Local f:Frame=image.frames[frame]
	
	PushMatrix

	Translate x,y
	Rotate rotation
	Scale scaleX,scaleY
	Translate -image.tx,-image.ty

	context.Validate
	
	renderDevice.DrawSurface2 image.surface,0,0,srcX+f.x,srcY+f.y,srcWidth,srcHeight

	PopMatrix
End

Function ReadPixels( pixels[],x,y,width,height,offset=0,pitch=0 )

	If Not pitch pitch=width

#If CONFIG="debug"
	DebugRenderDevice
	If x<0 Or y<0 Or x+width>DeviceWidth() Or y+height>DeviceHeight() Error "Invalid pixel rectangle"
	If offset<0 Or pitch<0 Or offset+(height-1)*pitch+width>pixels.Length Error "Invalid array rectangle"
#End

	renderDevice.ReadPixels pixels,x,y,width,height,offset,pitch
End

Function SetFont( font:Image,firstChar=32 )
	If Not font
		If Not context.defaultFont
			context.defaultFont=LoadImage( "mojo_font.png",96,Image.XPadding )
		Endif
		font=context.defaultFont
		firstChar=32
	Endif
	context.font=font
	context.firstChar=firstChar
End

Function GetFont:Image()
	Return context.font
End

Function TextWidth#( text$ )
	If context.font Return text.Length * context.font.Width
End

Function TextHeight#()
	If context.font Return context.font.Height
End

Function FontHeight#()
	If context.font Return context.font.Height
End

Function DrawText( text$,x#,y#,xalign#=0,yalign#=0 )
#If CONFIG="debug"
	DebugRenderDevice
#End
	If Not context.font Return
	
	Local w=context.font.Width
	Local h=context.font.Height
	
	x-=Floor( w * text.Length * xalign )
	y-=Floor( h * yalign )
	
	For Local i=0 Until text.Length
		Local ch=text[i]-context.firstChar
		If ch>=0 And ch<context.font.Frames
			DrawImage context.font,x+i*w,y,ch
		Endif
	Next

End

Function InvTransform#[]( coords#[] )
	Local m00#=   context.ix
	Local m10#=   context.jx
	Local m20#=   context.tx
	Local m01#=   context.iy
	Local m11#=   context.jy
	Local m21#=   context.ty
	Local det#=   m00*m11 - m01*m10
	Local idet#=  1.0/det
	Local r00# =  m11 * idet
	Local r10# = -m10 * idet
	Local r20# = (m10*m21 - m11*m20) * idet
	Local r01# = -m01 * idet
	Local r11# =  m00 * idet
	Local r21# = (m01*m20 - m00*m21) * idet
	'Local r22# = (m00*m11 - m01*m10) * idet		'what do I do with this?
	Local ix#=r00,jx#=r10,tx#=r20,iy#=r01,jy#=r11,ty#=r21
	Local out#[ coords.Length ]
	For Local i=0 Until coords.Length-1 Step 2
		Local x#=coords[i],y#=coords[i+1]
		out[i]=   x*ix + y*jx + tx
		out[i+1]= x*iy + y*jy + ty
	Next
	Return out
End

#Endif
