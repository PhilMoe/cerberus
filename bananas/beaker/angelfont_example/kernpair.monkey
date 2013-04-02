
Class KernPair
	Field first:String
	Field second:String
	Field amount:Int
	
	
	Method New(first:Int, second:Int, amount:Int)
		Self.first = first
		Self.second = second
		Self.amount = amount
	End
	
#rem	
	Method toString:String()
		Return "first="+String.FromChar(first)+" second="+String.FromChar(second)+" amount="+amount
'		Return "first="+first+" second="+second+" amount="+amount
	End Method
#end
	
End Class






