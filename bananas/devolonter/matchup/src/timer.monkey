Strict

Import mojo

Class Timer
	
Private
	Field duration:Float

	Field timeLeft:Int
	
	Field active:Bool = False
	
	Field lastTime:Int
	
	Field callback:TimerCompleteListener
	
Public
	Method Alarm:Void(time:Int, onComplete:TimerCompleteListener)
		duration = time
		timeLeft = time
		active = True
		callback = onComplete
		lastTime = Millisecs()
	End Method
	
	Method AddTime:Void(time:Int)
		timeLeft += time
		If (timeLeft > duration) timeLeft = duration
	End Method
	
	Method Update:Void()
		If (Not active) Return
	
		timeLeft -= (Millisecs() -lastTime)
		lastTime = Millisecs()
		
		If (timeLeft <= 0) Then
			callback.OnTimerComplete()
			active = False
		End if
	End Method
	
	Method Percent:Float() Property
		Return timeLeft / duration
	End Method

End Class

Interface TimerCompleteListener
	
	Method OnTimerComplete:Void()

End Interface