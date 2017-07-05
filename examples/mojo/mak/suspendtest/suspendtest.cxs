
#MOJO_AUTO_SUSPEND_ENABLED=True

Import mojo

Class MyApp Extends App

	Field creates

	Field suspends,suspend_ms,resume_ms
	
	Field tinkle:Sound,tinkle_loop

	Method OnCreate()

#If TARGET="flash"
		tinkle=LoadSound( "tinkle.mp3" )
#Else If TARGET="android"
		tinkle=LoadSound( "tinkle.ogg" )
#Else
		tinkle=LoadSound( "tinkle.wav" )
#Endif
		SetUpdateRate 15
	End
	
	Method OnUpdate()

		If TouchHit(0)
			Select (Int(TouchY(0))-(14*10-14))/28
			Case 0
				tinkle_loop=Not tinkle_loop
				If tinkle_loop
					PlaySound tinkle,0,1
				Else
					StopChannel 0
				Endif
			Case 1
				Error "Runtime Error"
			Case 2
				Error ""
			End
		Endif
	End
	
	Method OnRender()
		Cls
		DrawText "Audio:          "+tinkle_loop,0,0
		DrawText "Millisecs:      "+Millisecs,0,14*1
		DrawText "Creates:        "+creates,0,14*2
		DrawText "Suspends:       "+suspends,0,14*3
		DrawText "Last suspended: "+suspend_ms,0,14*4
		DrawText "Last resumed:   "+resume_ms,0,14*5
		DrawText "Time suspended: "+(resume_ms-suspend_ms),0,14*6

		DrawText "[ *****  Click to Toggle audio  ***** ]",DeviceWidth/2,14*10,.5,.5
		DrawText "[ ***** Click for Runtime error ***** ]",DeviceWidth/2,14*12,.5,.5
		DrawText "[ *****  Click for Null error   ***** ]",DeviceWidth/2,14*14,.5,.5
	End
	
	Method OnSuspend()
		suspends+=1
		suspend_ms=Millisecs
	End
	
	Method OnResume()
		resume_ms=Millisecs
	End
End

Function Main()
	New MyApp
End
