
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
		If i=0 Return " 0.00"
		If i>99 Return  "+1.00"
		If i<-99 Return "-1.00"
		If i>9 Return "+0."+i
		If i<-9 Return "-0."+(-i)
	 	If i>0 Return "+0.0"+i
	 	If i<0 Return "-0.0"+(-i)
	 	Return "!"
	End
	
	Method OnRender()
		Scale DeviceWidth/400.0,DeviceHeight/400.0
		Cls
		For Local port:=0 Until 4
			Local x:=port*100,y:=20
			DrawText "Port "+port,x,0
			DrawText "JoyX(0) "+Format( JoyX(0,port) ),x,y+0
			DrawText "JoyY(0) "+Format( JoyY(0,port) ),x,y+20
			DrawText "JoyZ(0) "+Format( JoyZ(0,port) ),x,y+40
			DrawText "JoyX(1) "+Format( JoyX(1,port) ),x,y+60
			DrawText "JoyY(1) "+Format( JoyY(1,port) ),x,y+80
			DrawText "JoyZ(1) "+Format( JoyZ(1,port) ),x,y+100
			DrawText "JOY_A       "+JoyDown(JOY_A,port),x,y+120
			DrawText "JOY_B       "+JoyDown(JOY_B,port),x,y+140
			DrawText "JOY_X       "+JoyDown(JOY_X,port),x,y+160
			DrawText "JOY_Y       "+JoyDown(JOY_Y,port),x,y+180
			DrawText "JOY_LB      "+JoyDown(JOY_LB,port),x,y+200
			DrawText "JOY_RB      "+JoyDown(JOY_RB,port),x,y+220
			DrawText "JOY_BACK    "+JoyDown(JOY_BACK,port),x,y+240
			DrawText "JOY_START   "+JoyDown(JOY_START,port),x,y+260
			DrawText "JOY_LEFT    "+JoyDown(JOY_LEFT,port),x,y+280
			DrawText "JOY_UP      "+JoyDown(JOY_UP,port),x,y+300
			DrawText "JOY_RIGHT   "+JoyDown(JOY_RIGHT,port),x,y+320
			DrawText "JOY_DOWN    "+JoyDown(JOY_DOWN,port),x,y+340
		Next
		
	End
	
End

Function Main()

	New MyApp
	
End
