
Import thread

Class MyThread Extends Thread

	Private
	
	Method Run:Void()
		For Local i:=0 Until 100
			Print i
		Next
	End
	
End

Function Main()

	Local thread:=New MyThread
	
	thread.Start

	thread.Wait
	
	Print "Thread finished!"
End
