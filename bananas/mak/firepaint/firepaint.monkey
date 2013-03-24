
'Firepaint redux!
'
'Really a multitouch test...

Import mojo

Global BigList:=New List<Spark>

Class Spark

	Field x#,y#,xv#,yv#,a#
	
	Method New( x#,y# )
		Self.x=x
		Self.y=y
		Local an#=Rnd(360),v#=Rnd(3,4)
		Self.xv=Cos(an)*v
		Self.yv=Sin(an)*v
		Self.a=1
	End
	
End

Class Firepaint Extends App

	Field sparkImage:Image
	Field sparks:=New List<Spark>
	
	Field prim
	
	Method OnCreate()
	
		For Local i=0 Until 65536*16
'			BigList.AddLast New Spark
		Next
		
		sparkImage=LoadImage( "bluspark.png",1,Image.MidHandle )
		
		SetUpdateRate 60
	End
	
	Method OnUpdate()
	
		Local out:=New List<Spark>
		For Local spark:=Eachin sparks
			spark.a=Max(spark.a-.01,0.0)
			If spark.a<=0 Continue
			spark.yv+=.05
			spark.x+=spark.xv
			spark.y+=spark.yv
			out.AddLast spark
		Next

		sparks=out
		
		For Local i=0 Until 32
			If TouchDown( i )
				For Local j=1 To 10
					sparks.AddLast New Spark( TouchX(i),TouchY(i) )
				next
			Endif
		Next
		
		If KeyHit( KEY_SPACE )
			prim=(prim+1) Mod 4
		Endif
		
	End
	
	Method OnRender()
	
		Local w=DeviceWidth,h=DeviceHeight
		
		SetBlend LightenBlend

		Cls
		
		DrawText "This way UP!",0,0
		
		SetColor 255,0,0
		DrawRect 0,0,w,32
		SetColor 0,255,0
		DrawRect w-32,0,32,h
		SetColor 0,0,255
		DrawRect 0,h-32,w,32
		SetColor 128,128,128
		DrawRect 0,0,32,h
		
		For Local spark:=Eachin sparks

			SetAlpha spark.a
		
			Select prim
			Case 0
				DrawImage sparkImage,spark.x,spark.y
			Case 1
				DrawRect spark.x-15,spark.y-15,30,30
			Case 2
				DrawOval spark.x-15,spark.y-15,30,30
			Case 3
				DrawLine spark.x-15,spark.y-15,spark.x+15,spark.y+15
				DrawLine spark.x+15,spark.y-15,spark.x-15,spark.y+15
			End
		Next
	End
	
End

Function Main()

	New Firepaint
	
End
