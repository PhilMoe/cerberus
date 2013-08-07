
#ANDROID_GAMEPAD_ENABLED=True

Import mojo

Class MyApp Extends App

	Method OnCreate()
		SetUpdateRate 15
	End
	
	Method OnUpdate()
	End
	
	Method OnRender()
		Cls
		DrawText "JoyX(0)            "+JoyX(0),0,0
		DrawText "JoyY(0)            "+JoyY(0),0,20
		DrawText "JoyZ(0)            "+JoyZ(0),0,40
		DrawText "JoyX(1)            "+JoyX(1),0,60
		DrawText "JoyY(1)            "+JoyY(1),0,80
		DrawText "JoyZ(1)            "+JoyZ(1),0,100
		DrawText "JoyDown(JOY_A)     "+JoyDown(JOY_A),0,120
		DrawText "JoyDown(JOY_B)     "+JoyDown(JOY_B),0,140
		DrawText "JoyDown(JOY_X)     "+JoyDown(JOY_X),0,160
		DrawText "JoyDown(JOY_Y)     "+JoyDown(JOY_Y),0,180
		DrawText "JoyDown(JOY_LB)    "+JoyDown(JOY_LB),0,200
		DrawText "JoyDown(JOY_RB)    "+JoyDown(JOY_RB),0,220
		DrawText "JoyDown(JOY_BACK)  "+JoyDown(JOY_BACK),0,240
		DrawText "JoyDown(JOY_START) "+JoyDown(JOY_START),0,260
		DrawText "JoyDown(JOY_LEFT)  "+JoyDown(JOY_LEFT),0,280
		DrawText "JoyDown(JOY_UP)    "+JoyDown(JOY_UP),0,300
		DrawText "JoyDown(JOY_RIGHT) "+JoyDown(JOY_RIGHT),0,320
		DrawText "JoyDown(JOY_DOWN)  "+JoyDown(JOY_DOWN),0,340
		
	End
	
End

Function Main()

	New MyApp
	
End
