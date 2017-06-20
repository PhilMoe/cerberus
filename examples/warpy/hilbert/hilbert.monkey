
Import mojo

Class HilbertApp Extends App
	Method OnCreate()
		SetUpdateRate 3
	End

	'Virtual resolution code nicked from Mak
	Field vres_transx#=0,vres_transy#=0,vres_width#,vres_height#,vres_scalex#=1,vres_scaley#=1
	Method SetVirtualResolution( width#,height# )
		Local dwidth#=DeviceWidth
		Local dheight#=DeviceHeight

		If (dwidth/dheight)>(width/height)
			vres_width=dheight*(width/height)
			vres_height=dheight
		Else
			vres_width=dwidth
			vres_height=dwidth*(height/width)
		Endif
		
		vres_scalex=vres_width/width
		vres_scaley=vres_height/height
		vres_transx=(dwidth-vres_width)/2
		vres_transy=(dheight-vres_height)/2
		
		SetScissor vres_transx,vres_transy,vres_width,vres_height
		Translate vres_transx,vres_transy
		Scale vres_scalex,vres_scaley
	End

	Field n
	Method OnRender()
		SetVirtualResolution 610,610
		Translate 5,5
		Cls 0,0,0
		SetColor 255,255,255

		hilbert(n,90,600.0/(Pow(2,n)-1))
		n = n Mod 7 +1 
	End

	Method hilbert(level,angle,size#)
		If level=0 Return

		Rotate -angle				'turn right
		hilbert level-1,-angle,size	'recur
		DrawLine 0,0,size,0			'move forwards
		Translate size,0
		Rotate angle				'turn left
		hilbert level-1,angle,size	'recur
		DrawLine 0,0,size,0			'move forwards
		Translate size,0
		hilbert level-1,angle,size	'recur
		Rotate angle				'turn left
		DrawLine 0,0,size,0			'move forwards
		Translate size,0
		hilbert level-1,-angle,size	'recur
		Rotate -angle				'turn right
	End
End

Function Main()
	New HilbertApp()
End

