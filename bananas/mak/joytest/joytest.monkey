
#ANDROID_GAMEPAD_ENABLED=True

Import mojo

Class MyApp Extends App

	Method OnCreate()
		SetUpdateRate 15
	End
	
	Method OnUpdate()
	End
	
	Method Format:String( n:Float )
		Local i:=Int( n*100 )
		If i>99 Return  "+1.00"
		If i<-99 Return "-1.00"
		If i>9 Return "+0."+i
		If i<-9 Return "-0."+(-i)
	 	If i>0 Return "+0.0"+i
	 	If i<0 Return "-0.0"+(-i)
	 	Return " 0.00"
	End
	
	Method OnRender()
		Scale DeviceWidth/400.0,DeviceHeight/440.0
		Cls
		For Local port:=0 Until 4
			If JoyHit( JOY_MENU,port ) Print "Bong!"			
			PushMatrix
			Translate port*100,0
			DrawText "Port "+port,0,0
			DrawText "JoyX(0) "+Format( JoyX(0,port) ),0,20
			DrawText "JoyY(0) "+Format( JoyY(0,port) ),0,40
			DrawText "JoyZ(0) "+Format( JoyZ(0,port) ),0,60
			DrawText "JoyX(1) "+Format( JoyX(1,port) ),0,80
			DrawText "JoyY(1) "+Format( JoyY(1,port) ),0,100
			DrawText "JoyZ(1) "+Format( JoyZ(1,port) ),0,120
			DrawText "JOY_A       "+JoyDown(JOY_A,port),0,140
			DrawText "JOY_B       "+JoyDown(JOY_B,port),0,160
			DrawText "JOY_X       "+JoyDown(JOY_X,port),0,180
			DrawText "JOY_Y       "+JoyDown(JOY_Y,port),0,200
			DrawText "JOY_LB      "+JoyDown(JOY_LB,port),0,220
			DrawText "JOY_RB      "+JoyDown(JOY_RB,port),0,240
			DrawText "JOY_BACK    "+JoyDown(JOY_BACK,port),0,260
			DrawText "JOY_START   "+JoyDown(JOY_START,port),0,280
			DrawText "JOY_LEFT    "+JoyDown(JOY_LEFT,port),0,300
			DrawText "JOY_UP      "+JoyDown(JOY_UP,port),0,320
			DrawText "JOY_RIGHT   "+JoyDown(JOY_RIGHT,port),0,340
			DrawText "JOY_DOWN    "+JoyDown(JOY_DOWN,port),0,360
			DrawText "JOY_LSB     "+JoyDown(JOY_LSB,port),0,380
			DrawText "JOY_RSB     "+JoyDown(JOY_RSB,port),0,400
			DrawText "JOY_MENU    "+JoyDown(JOY_MENU,port),0,420
			PopMatrix
		Next
	End

End

Function Main()

	New MyApp
	
End
