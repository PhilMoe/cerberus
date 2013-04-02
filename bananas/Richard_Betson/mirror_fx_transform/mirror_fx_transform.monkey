'*****************************************************
'* Mirror FX using Transform 
'* Author: Richard Betson
'* Date: 02/09/11
'* Language: monkey
'* Tagets: HTML5, FLASH, GLFW
'* License - Public Domain
'*****************************************************

Import mojo

Global img:Image

Class MyApp Extends App

	Global t#
	Global tt#
	Global ttx#
	Global rt#
	Global rtt#
	
	Method OnCreate()
		SetUpdateRate 60
		img=LoadImage("c.png")
	End Method

	Method OnUpdate()
		rtt=rtt+1
		If KeyDown(KEY_RIGHT) Then t=t+.001
		If KeyDown(KEY_LEFT) Then t=t-.001
		If KeyDown(KEY_UP) Then tt=tt+.0001
		If KeyDown(KEY_DOWN) Then tt=tt-.0001
		If KeyDown(KEY_1) Then ttx=ttx+.0001
		If KeyDown(KEY_2) Then ttx=ttx-.0001

	End Method

	Method OnRender()
		Cls
		PushMatrix()
		Local stp=0
		Local clr#
		Local oldx=20
		Local oldy=10

		For Local ii=0 To 200 
		Local i=200-ii
		clr=clr+.004
		If clr>=1 Then clr=1
	
		stp=stp+1
		If stp>=30
			stp=0
			SetColor((i)+55,0,255-i)
			
			For Local y=10 To 480 Step 110
			SetAlpha (1-clr)

				For Local x=20 To 640 Step 120
					DrawLine oldx,y,x,y
					DrawLine x,oldy,x,y
					oldx=x
					DrawCircle(x,y,(8-(ii*.01)))
				Next
				oldx=20
				oldy=y
			Next
		Endif
		Translate(2,2)
		Transform 1+Sin(t-.259)-ttx-.001,0,0,Cos(t-.259)-tt-.007,Cos(rtt),Sin(rtt)
		
		
		Next
		Translate(0,0)
		Transform 1,0,0,1,0,0
		PopMatrix()

		oldx=20
		SetColor 255,0,0
		SetAlpha 1
		For Local y=10 To 480 Step 110
			For Local x=20 To 640 Step 120
					DrawLine oldx,y,x,y
					DrawLine x,oldy,x,y
					oldx=x
				DrawImage(img,x-9,y-7,1,1,1)
			Next
			oldx=20
			oldy=y
		Next
	
		SetColor 255,255,255
		
	End Method
	
End Class

Function Main()

	New MyApp
	
End
