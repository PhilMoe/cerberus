
' Module monkey.Stack
'
' Placed into the public domain 24/02/2011.
' No warranty implied; use at your own risk.

Class Stack<T>

	Method New()
	End
	
	Method New( data:T[] )
		Self.data=data[..]
		Self.length=data.Length
	End
	
	Method ToArray:T[]()
		Local t:T[length]
		For Local i=0 Until length
			t[i]=data[i]
		Next
		Return t
	End

	Method Equals?( lhs:T,rhs:T )
		Return lhs=rhs
	End
	
	Method Compare( lhs:T,rhs:T )
		Error "Unable to compare items"
	End
	
	Method Clear()
		length=0
	End

	Method Length()
		Return length
	End

	Method IsEmpty?()
		Return length=0
	End
	
	Method Contains?( value:T )
		For Local i=0 Until length
			If Equals( data[i],value ) Return True
		Next
	End Method
	
	Method Push( value:T )
		If length=data.Length
			data=data.Resize( length*2+10 )
		Endif
		data[length]=value
		length+=1
	End
	
	Method Push( values:T[],offset:Int=0 )
		For Local i:=offset Until values.Length
			Push values[i]
		Next
	End

	Method Push( values:T[],offset:Int,count:Int )
		For Local i:=0 Until count
			Push values[offset+i]
		Next
	End

	Method Pop:T()
		length-=1
		Return data[length]
	End
	
	Method Top:T()
		Return data[length-1]
	End

	Method Set( index,value:T )
		data[index]=value
	End

	Method Get:T( index )
		Return data[index]
	End

	Method Insert( index,value:T )
		If length=data.Length
			data=data.Resize( length*2+10 )
		Endif
		For Local i=length Until index Step -1
			data[i]=data[i-1]
		Next
		data[index]=value
		length+=1
	End

	Method Remove( index )
		For Local i=index Until length-1
			data[i]=data[i+1]
		Next
		length-=1
	End
	
	Method RemoveEach( value:T )
		Local i
		While i<length
			If Not Equals( data[i],value )
				i+=1
				Continue
			Endif
			Local b=i,e=i+1
			While e<length And Equals( data[e],value )
				e+=1
			Wend
			While e<length
				data[b]=data[e]
				b+=1
				e+=1
			Wend
			length-=e-b
			i+=1
		Wend
	End
	
	Method ObjectEnumerator:Enumerator<T>()
		Return New Enumerator<T>( Self )
	End
	
	Method Backwards:BackwardsStack<T>()
		Return New BackwardsStack<T>( Self )
	End
	
Private

	Field data:T[]
	Field length
	
End

Class Enumerator<T>

	Method New( stack:Stack<T> )
		Self.stack=stack
	End

	Method HasNext:Bool()
		Return index<stack.Length
	End

	Method NextObject:T()
		index+=1
		Return stack.Get( index-1 )
	End

Private

	Field stack:Stack<T>
	Field index

End

Class BackwardsStack<T>

	Method New( stack:Stack<T> )
		Self.stack=stack
	End

	Method ObjectEnumerator:BackwardsEnumerator<T>()
		Return New BackwardsEnumerator<T>( stack )
	End Method
	
Private

	Field stack:Stack<T>

End

Class BackwardsEnumerator<T>

	Method New( stack:Stack<T> )
		Self.stack=stack
		index=stack.length
	End Method

	Method HasNext:Bool()
		Return index>0
	End 

	Method NextObject:T()
		index-=1
		Return stack.Get( index )
	End

Private
	
	Field stack:Stack<T>
	Field index

End

'Helper versions

Class IntStack Extends Stack<Int>

	Method New( data:Int[] )
		Super.New( data )
	End
	
	Method Equals?( lhs,rhs )
		Return lhs=rhs
	End
	
	Method Compare( lhs,rhs )
		Return lhs-rhs
	End

End

Class FloatStack Extends Stack<Float>
	
	Method New( data:Float[] )
		Super.New( data )
	End
	
	Method Equals?( lhs#,rhs# )
		Return lhs=rhs
	End
	
	Method Compare( lhs#,rhs# )
		If lhs<rhs Return -1
		Return lhs>rhs
	End
	
End

Class StringStack Extends Stack<String>

	Method New( data:String[] )
		Super.New( data )
	End
	
	Method Join$( separator$="" )
		Return separator.Join( ToArray() )
	End
	
	Method Equals?( lhs$,rhs$ )
		Return lhs=rhs
	End

	Method Compare( lhs$,rhs$ )
		Return lhs.Compare( rhs )
	End

End
