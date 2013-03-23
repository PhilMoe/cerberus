
Class Range<T>

	'for all ranges
	Method Count:Int()
	Method IsEmpty:Bool()
	
	'for forward ranges
	Method GetFront:T()
	Method PopFront:Void()
	
	'for bidirectional ranges
	Method GetBack:T()
	Method PopBack:Void()
	
	'for random access ranges
	Method Length:Int()
	Method GetElement:T( index:Int )
	
End

Class ListRange<T>

	Field _begin:Node<T>
	Field _end:Node<T>
	
	Method Count:Int()
	End
	
	Method 
End

Class Container<T>

	Method All:Range<T>
	
End

