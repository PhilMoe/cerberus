
#MOJO_AUTO_SUSPEND_ENABLED=True

Import monkeytarget

Class GameDelegate Extends BBGameDelegate

	Method StartGame:Void()
		Print "StartGame"
		BBGame.Game().SetUpdateRate 60
	End
	
	Method SuspendGame:Void()
		Print "SuspendGame"
	End
	
	Method ResumeGame:Void()
		Print "ResumeGame"
	End
	
	Method UpdateGame:Void()
		Print "UpdateGame"
	End
	
	Method RenderGame:Void()
		Print "RenderGame"
	End
	
	Method KeyEvent:Void( event:Int,data:Int )
		Print "KeyEvent: event="+event+", data="+data
	End
	
	Method MouseEvent:Void( event:Int,data:Int,x:Float,y:Float )
		Print "MouseEvent: event="+event+", data="+data+", x="+x+", y="+y
	End
	
	Method TouchEvent:Void( event:Int,data:Int,x:Float,y:Float )
		Print "TouchEvent: event="+event+", data="+data+", x="+x+", y="+y
	End

	Method MotionEvent:Void( event:Int,data:Int,x:Float,y:Float,z:Float )
		Print "MotionEvent: event="+event+", data="+data+", x="+x+", y="+y+", z="+z
	End
	
	Method DiscardGraphics:Void()
		Print "DiscardGraphics"
	End

End

Function Main()

	BBGame.Game().SetDelegate New GameDelegate

End
