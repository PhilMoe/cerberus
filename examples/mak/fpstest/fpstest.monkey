
Import mojo

Class MyApp Extends App

	Field x#
	Field count=1
	Field deltaTime#=1
	
	Field updateRate=60
	Field updateMillis,millisAcc
	
	Method OnCreate()
		SetUpdateRate updateRate
	End
	
	Method OnUpdate()
	
		If updateMillis
			'Bresenham thing...
			Local period=1000/updateRate
			millisAcc+=1000-period*updateRate
			If millisAcc>=updateRate
				period+=1
				millisAcc-=updateRate
			Endif
			updateMillis+=period
		Else
			updateMillis=Millisecs()
		End
	
		x+=deltaTime	'constant velocity
		If x>DeviceWidth x=0
		
		If KeyDown(KEY_UP)
			count+=10
		Else If KeyDown(KEY_DOWN)
			count-=10
			If count<1 count=1
		Endif
		
		If KeyHit(KEY_LEFT)		'drop to a lower update rate
			If updateRate>15
				updateRate/=2
				deltaTime*=2
				SetUpdateRate updateRate
				updateMillis=0
				millisAcc=0
			Endif
		Else If KeyHit(KEY_RIGHT)	'up to a higher update rate
			If updateRate<120
				updateRate*=2
				deltaTime/=2
				SetUpdateRate updateRate
				updateMillis=0
				millisAcc=0
			Endif
		Endif

	End
	
	Method OnRender()
		Local err=Millisecs()-updateMillis
		For Local i=1 To count
			Cls
			DrawRect x,0,100,100
			DrawText "count="+count,0,0
			DrawText "updateRate="+updateRate,0,14
			DrawText "Time Error="+err,0,28
		Next
	End

End

Function Main()
	New MyApp
End
