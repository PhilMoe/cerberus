
Import mojo

Class Horse
	Field foot,trot#,dir,clop
	Field speed#,steps,laststop
	Field x#,y#

	Method New()
		dir=1
	End

	Method Update(ms#)
		speed = (KeyDown(KEY_RIGHT) - KeyDown(KEY_LEFT))*10
		If speed<>0
			dir = Sgn(speed)
			If ms>trot
				trot += .2
				x += speed
				foot = (foot Mod 3) + 1
				steps += 1
				If foot = 1
					PlaySound myapp.data.clops[clop]
					clop = (clop Mod 2) + 1
				Endif
			Endif
			laststop = ms
		Else
			If ms>trot
				trot = ms
				foot = 0
				clop = 0
				If steps>10 And Rnd(150)<steps
					PlaySound(myapp.data.neigh)
				Endif
				steps = 0
			Endif

			'If ms-laststop>2
			'	SetChannelVolume bgchannel,(ms-laststop)/2.0
			'Else
			'	SetChannelVolume bgchannel,1
			'Endif
		Endif
	End

	Method Draw()
		Local xoff#,yoff#,rot#
		xoff=-20

		Select foot
		Case 0
			yoff = -33
			rot = 0
		Case 1
			xoff -= 10
			yoff = -Cos(20)*33
			rot = -20
		Case 3
			xoff += 10
			yoff = -Cos(20)*33-Sin(20)*40
			rot = 20
		Case 2
			yoff = -35
			rot = 0
		End

		DrawZoomImage myapp.data.horseimg,x+xoff*.2,y-40+yoff*.2,rot,.2,.2
	End 
	
End

Class Data
	Field horseimg:Image
	Field bg:Image,fg:Image

	Field clops:Sound[3]
	Field neigh:Sound
	Field ambient:Sound
	
	Method New()
		Image.DefaultFlags = Image.MidHandle

		horseimg = LoadImage( "horse.png" )

		bg = LoadImage( "bg.png" )
		fg = LoadImage( "fg.png" )

		clops[0]=LoadSound( "clop1.mp3" )
		clops[1]=LoadSound( "clop2.mp3" )
		clops[2]=LoadSound( "clop3.mp3" )
		
		neigh = LoadSound("neigh.mp3")
		
		ambient = LoadSound("bg.mp3")
		PlaySound ambient
	End
End

Class Horsey Extends App
	Field ms#

	Field vpanx#,vpany#

	Field horse:Horse

	Field data:Data


	Method OnCreate()
		
		data = New Data()

		SetUpdateRate 60

		horse = New Horse()
	End

	Method OnUpdate()
		ms=ms+1.0/30.0

		horse.Update(ms)

		Local dpan# = (horse.x - panx)*.01
		vpanx += dpan+Rnd(-.1,.1)
		vpany += Rnd(-1,1)*Abs(dpan)*.2+Rnd(-.1,.1)
		vpanx = vpanx * .8
		vpany = vpany * .8
		panx += vpanx
		pany += vpany

	End

	Field tw,th

	Method OnRender()
		Local w=DeviceWidth
		Local h=DeviceHeight

		If w<>tw Or h<>th
			tw=w
			th=h
		Endif

		Cls 0,0,0

		'Starting fullscreen effects - don't forget to pop later!
		PushMatrix

		'scale 640,480 to device size - ie: virtual resolution handling!
		Scale( tw/600.0,th/450.0 )

		Local scroll# = (-panx) Mod 600
		DrawImage data.bg,scroll+300,-pany+300-25
		DrawImage data.bg,scroll-300,-pany+300-25
		DrawImage data.bg,scroll+900,-pany+300-25

		horse.Draw()

		DrawImage data.fg,scroll+300,-pany+300-25
		DrawImage data.fg,scroll-300,-pany+300-25
		DrawImage data.fg,scroll+900,-pany+300-25

		PopMatrix
	End		

End

Global myapp:Horsey

Function Main()

	myapp = New Horsey

End

Global panx#,pany#
Function DrawZoomImage( i:Image, x#,y#, rot#, sx#,sy# )
	DrawImage i,x-panx+300,y-pany+350,rot,sx,sy
End

