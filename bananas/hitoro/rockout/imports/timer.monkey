
Import rockout

Class Timer

	Global NewGame:Timer
	Global ShotReload:Timer
	
	Field ticks:Int
	
	Method New ()
		Self.ticks = Millisecs ()
	End

	Method TimeOut (timeout:Int)
		If Millisecs () > ticks + timeout
			Return True
		End
	End
	
	Method Reset ()
		ticks = Millisecs ()
	End
	
End
