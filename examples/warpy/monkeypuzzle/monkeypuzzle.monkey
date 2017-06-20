
Import mojo

Class MonkeyPuzzle Extends App

	Field tw,th
	Field time#

	Field monkey:Image

	Field levels

	Method OnCreate()
		Image.DefaultFlags = Image.MidHandle

		monkey = LoadImage( "monkey.png" )

		levels=1

		MonkeyReport
		Print "Press up for more monkey madness!"

		SetUpdateRate 60
	End

	Method OnUpdate()
		time += 1.0/60.0

		If KeyHit(KEY_UP)
			levels += 1
			MonkeyReport
		Endif
		If KeyHit(KEY_DOWN) And levels>1	'exercise for the reader: why does 'KeyHit' need to come before 'levels>1' here?
			levels -= 1
			MonkeyReport
		Endif
	End

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
		Scale( tw/640.0,th/480.0 )


		'Start at the middle of the screen 
		Translate 320,240
		
		'Moving the mouse will make the view move and zoom in, for enhanced mad monkey movement
		Local dx# = MouseX()-320
		Local dy# = MouseY()-240

		Local d# = Sqrt(dx*dx + dy*dy)	'get the distance of the mouse from the centre of the screen
		Local f# = levels*d/200 + 1 		'work out the zoom factor.
										'By multiplying by the number of levels of monkeys, we will be able to see all the cheeky little monkeys!
										'multiplying by the distance and adding 1 means that when the mouse is in the centre of the screen, everything looks normal
		Translate dx*levels,dy*levels
		Scale f,f		'perform the Scale transformation by the calculated factor

		Rotate 90+time*60	'rotate everything!


		'Start the monkey madness!
		MonkeyMadness levels,4

		PopMatrix

	End
	
	'this is a recursive process - it uses PushMatrix and PopMatrix to produce many different transformations based on the same starting position
	'n is the number of levels of monkeys to draw underneath this one. If n=0 then no monkey is drawn
	'the turns parameter is there because at the first level you have four sub-monkeys per monkey, then it's three on lower levels
	Method MonkeyMadness(n,turns=3)	
		If n=0 Return

		Local distance# = 400+Cos(time*131)*200/n	'The distance between this layer of monkeys and the next. It changes with time, and is less at bigger levels
													'It is also affected by the Scale transformations in the higher levels

		For Local c=1 To turns
			PushMatrix						'remember the initial transformation, we'll need to base the next monkey off it

			Rotate((c+2)*90)					'rotate by 90 degrees
			Scale .5,.5						'shrink by a half
			Translate distance,0			'move "forwards" a bit. Because we've rotated, what this really does is move away from the start position in the desired direction
			MonkeyMadness n-1				'repeat the process using this new transformation

			PopMatrix						'get the starting transformation back so we can do another turn, or to draw the big monkey at the end
		Next

		Local wobble# = -n*Cos(300*time/n)*120
		DrawImage monkey,0,0,wobble,0.7,0.7		'Draw a monkey, using the initial transformation matrix.
														'The transformation matrix is back to where it began because we popped it off the stack after drawing the sub-monkeys
	End

	'how many mad monkeys are making matrix multiplications at the moment?
	Method MonkeyReport()
		Select levels
		Case 1
			Print "1 mad monkey making matrix multiplications."
		Case 2
			Print "5 mad monkeys making matrix multiplications."
		Default
			Local tot = 5	'five monkeys who don't obey the rules
			Local num = 4	'four little monkeys hanging off the biggest monkey
			Local punctuation$=""
			For Local c=3 To levels
				num = num * 3	'three smaller monkeys hanging off each bigger monkey
				tot += num
				punctuation += "!"	'this is getting more and more mad!
			Next

			Print tot+" mad monkeys making matrix multiplications"+punctuation
		End
	End
End

Global myapp:MonkeyPuzzle
Function Main()
	myapp = New MonkeyPuzzle()
End



