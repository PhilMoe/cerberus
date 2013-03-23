
Class Pool<T>

	Method New( initialCapacity:Int=10 )
		For Local i:=0 Until initialCapacity
			_pool.Push New T
		Next
	End

	Method Allocate:T()
		If _pool.IsEmpty() Return New T
		Return _pool.Pop()
	End
	
	Method Free:Void( t:T )
		_pool.Push t
	End
	
	Private
	
	Field _pool:=New Stack<T>

End

