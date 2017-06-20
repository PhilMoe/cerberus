'******************************************************************
'*	Tile Image Example
'*	Author: Richard R Betson
'*	Date: 01/23/11
'*	Language: monkey
'*  Tagets: HTML5, FLASH, GLFW
'*  License - Public Domain							  
'******************************************************************

Import mojo
 
Function Main()
	New TileImage
End Function


Class TileImage Extends App
Field img:Image
Field ix#,iy#
Field scale_width#=1.0,scale_hieght#=1.0

	Method OnCreate()
		SetUpdateRate(60)
		img=LoadImage( "bg.png" )
		SetFont Null
	End Method
	
	Method OnUpdate()
	
		If KeyDown (KEY_LEFT)
			scale_width = scale_width - 0.1
			If scale_width < 1.0 Then scale_width = 1.0
		Endif
		If KeyDown (KEY_RIGHT)
			scale_width = scale_width + 0.1
		Endif
		If KeyDown (KEY_DOWN)
			scale_hieght = scale_hieght - 0.1
			If scale_hieght < 1.0 Then scale_hieght = 1.0
		Endif
		If KeyDown (KEY_UP)
			scale_hieght = scale_hieght + 0.1
		Endif
		
	End Method
	
	Method OnRender()
		Local ih=128,iw=128
		Local scale_x#=.99	'Some browsers like FireFox need a .01 offset to display right
		Local scale_y#=.99

		'Cls 0,0,0
		PushMatrix()
		
		Translate DeviceWidth * 0.5, DeviceHeight * 0.5 'Thanks BlitzSupport:)
		Scale scale_width,scale_hieght
		Translate -DeviceWidth * 0.5, -DeviceHeight * 0.5' ""

		ix=ix+1.5
		iy=iy+1
		Tile_Image(img,ix,iy,ih,iw,scale_x,scale_y)
		PopMatrix()

		PushMatrix()
		Translate(0,0)
		Scale (1,1)
		SetBlend 0
		DrawText("Use Arrow Keys to adjust scale.",10,10)

		PopMatrix()

	End Method
	


Function Tile_Image(img:Image,x#,y#,ih,iw,scalex#,scaley#)
'Based in part on - http://www.blitzbasic.com/codearcs/codearcs.php?code=1842 
	Local w#=iw * scalex
	Local h#=ih * scaley

	Local scissor:Float[]
	scissor=GetScissor()
	Local viewport_x=scissor[0]
	Local viewport_y=scissor[1]
	Local viewport_w=scissor[2]
	Local viewport_h=scissor[3]
	
	Local ox#=viewport_x-w+1
	Local oy#=viewport_y-h+1

    Local px#=x
    Local py#=y

    Local fx#=px-Floor(px)
    Local fy#=py-Floor(py)
    Local tx#=Floor(px)-ox
    Local ty#=Floor(py)-oy

    If tx>=0 tx=tx Mod w + ox Else tx = w - -tx Mod w + ox
    If ty>=0 ty=ty Mod h + oy Else ty = h - -ty Mod h + oy

    Local vr#= viewport_x + viewport_w, vb# = viewport_y + viewport_h

	Local iy#=ty
	While iy<(vb + h) 
        Local ix#=tx
        While ix<(vr + w) 
            DrawImage(img,ix+fx,iy+fy)
            ix=ix+w
        Wend
        iy=iy+h
    Wend
End Function


End Class


