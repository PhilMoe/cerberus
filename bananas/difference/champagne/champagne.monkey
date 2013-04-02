' 2010.09.21 by Difference
'
'Press space to add 1000 dots.
'Flash version performs really well even and easyly does 20000 dots.
'HTML5 version seems to be approximately 20 times slower and lags on more than 1000 dots.
'Inspired by the Blitzmax version http://blitzmax.com/Community/posts.php?topic=74667#835464 

Import mojo


Global w:Int = 640
Global h:Int = 480


Class Dot

	Field b:Int
	Field x:Float,y:Float
	Field a:Float,e:Float

	Method New()
		Self.b=Rnd(255)
		While a=0 Or e=0
			Self.a = Rnd(-1,1)
			Self.e = Rnd(-5,0)
		Wend
		x = 320 + Rnd(-120,120)
		y = 0
	End Method

	
	Method Update()
		x = x+a
		y = y+e
	
		If y>h y=y-h
		If y<0 y=y+h	
		

		If y > 460
			Local lim:Int = 120
			
			If x>320 +lim Then x = x-2*lim
			If x<320 -lim Then x = x+2*lim	
				
		Else If y > 240
			Local lim:Int = 10
			
			If x>320 +lim Then x = x-2*lim
			If x<320 -lim Then x = x+2*lim	
				

		Else
			Local lim:Int = 10 + 240 - y 
			
			If x>320 +lim Then x = x-2*lim
			If x<320 -lim Then x = x+2*lim	

		Endif
		
		
	End Method
	
	Method Draw()
		SetColor 255,255,b 
	'	DrawPoint(Self.x,Self.y)
		DrawRect(x,y,1,1)
	
	End Method

End


Class Champange Extends App

	Field dots:List<Dot> = New List<Dot>
	
	Method OnCreate()
			
		Local d:Dot 
	
		For Local n:Int = 0 To 1000
			d = New Dot()
			Self.dots.AddLast(d)
		Next

		SetUpdateRate 30
		Print  "Use spacebar to add 1000 dots"
	End
	
	Method OnUpdate()
	
		If KeyHit( KEY_SPACE )
		
			Local d:Dot 
		
			For Local n:Int = 0 To 1000
				d = New Dot()
				Self.dots.AddLast(d)
			Next
		Endif	
	
	
		For Local d:Dot = Eachin dots
			d.Update()
		Next
	End Method


	Method OnRender()
		Cls 0,0,0
		
		For Local dot:=Eachin dots
			dot.Draw()
			
		Next
	End

End

Function Main()
	New Champange
End

