' 2010.09.24 by Difference

Import mojo

Class Clock Extends App

	Field adjust:Int

	Method OnCreate()

		SetUpdateRate 10
		Print  "Use arrows to ajust the clock"

	End

	Method OnUpdate()

		If KeyHit( KEY_UP )		'minus 1 minute
				adjust = adjust + 1000 * 60
		Else If KeyHit( KEY_DOWN )	'minus 1 minute
				adjust = adjust - 1000 * 60
		Else If KeyHit( KEY_LEFT )	'plus 1 hour
				adjust = adjust - 1000 * 60 *60
		Else If KeyHit( KEY_RIGHT )	'minus 1 hour
				adjust = adjust + 1000 * 60 *60
		Else If KeyHit( KEY_SPACE )	'plus a second
				adjust = adjust + 1000 / 60 
		Endif

	End


	Method OnRender()
	
		Local nowtime:Float =   Millisecs + adjust

		Cls 128,0,255

		Local w:Int = 640
		Local h:Int = 480
		
		Local r:Float = 0.95 * Min(w,h) / 2.0
	
		PushMatrix
	
			Translate w/2.0,h/2.0
		
			PushMatrix
				For Local a:Float = 0 To 12 
					DrawLine 0,r*0.8,0,r
					Rotate 360.0 / 12
				Next
			PopMatrix
		
			Local rot:Float =  nowtime * 0.006	
			Local mrot:Float = nowtime * 0.0001 
			Local hrot:Float = mrot / 12.0
			
			'hours
			PushMatrix
				Rotate -hrot
				SetColor 255,128,0
				DrawRect -6,0,12,-h/3
			PopMatrix
			
			DrawCircle 0,0, r / 10.0
	
			PushMatrix
				Rotate -mrot
				SetColor 255,255,0
				DrawRect -3,0,6,-h/2.5
			PopMatrix
			
			DrawCircle 0,0, r / 15.0
			
			PushMatrix
				Rotate -rot 
				SetColor 255,0,0
				DrawLine 0,0,0,-r
			PopMatrix
	
			DrawCircle 0,0, r / 20.0 
			
			Translate -w/2.0,-h/2.0

		PopMatrix

	End

	

	Method OnLoading()

		Print "Loading!"

	End

End

Function Main()

	New Clock

End

