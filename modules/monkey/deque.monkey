
Class Deque<T>

	Method New()
	End
	
	Method New( arr:T[] )
		_data=arr[..]
		_capacity=_data.Length
		_last=_capacity
	End

	Method Clear:Void()
		If _first<=_last
			For Local i:=_first Until _last
				_data[i]=NIL
			Next
		Else
			For Local i:=0 Until _last
				_data[i]=NIL
			Next
			For Local i:=_first Until _capacity
				_data[i]=NIL
			Next
		Endif
		_first=0
		_last=0
	End
	
	Method Length:Int() Property
		If _last>=_first Return _last-_first
		Return _capacity-_first+_last
	End
	
	Method IsEmpty:Bool() Property
		Return _first=_last
	End
	
	Method ToArray:T[]()
		Local data:T[Length]
		If _first<=_last
			For Local i:=_first Until _last
				data[i-_first]=_data[i]
			Next
		Else
			Local n:=_capacity-_first
			For Local i:=0 Until n
				data[i]=_data[_first+i]
			Next
			For Local i:=0 Until _last
				data[n+i]=_data[i]
			Next
		Endif
		Return data
	End
	
	Method ObjectEnumerator:Enumerator<T>()
		Return New Enumerator<T>( Self )
	End
	
	Method Get:T( index:Int )
#If CONFIG="debug"
		If index<0 Or index>=Length Error "Illegal deque index"
#Endif
		Return _data[(index+_first)Mod _capacity]
	End
	
	Method Set:Void( index:Int,value:T )
#If CONFIG="debug"
		If index<0 Or index>=Length Error "Illegal deque index"
#Endif
		_data[(index+_first)Mod _capacity]=value
	End
	
	Method PushFirst:Void( value:T )
		If Length+1>=_capacity Grow
		_first-=1
		If _first<0 _first=_capacity-1
		_data[_first]=value
	End
	
	Method PushLast:Void( value:T )
		If Length+1>=_capacity Grow
		_data[_last]=value
		_last+=1
		If _last=_capacity _last=0
	End
	
	Method PopFirst:T()
#If CONFIG="debug"
		If IsEmpty Error "Illegal operation on empty deque"
#Endif
		Local v:=_data[_first]
		_data[_first]=NIL
		_first+=1
		If _first=_capacity _first=0
		Return v
	End
	
	Method PopLast:T()
#If CONFIG="debug"
		If IsEmpty Error "Illegal operation on empty deque"
#Endif
		If _last=0 _last=_capacity
		_last-=1
		Local v:=_data[_last]
		_data[_last]=NIL
		Return v
	End
	
	Method First:T()
#If CONFIG="debug"
		If IsEmpty Error "Illegal operation on empty deque"
#Endif
		Return _data[_first]
	End
	
	Method Last:T()
#If CONFIG="debug"
		If IsEmpty Error "Illegal operation on empty deque"
#Endif
		Return _data[(_last-1)Mod _capacity]
	End
	
	Private
	
	Global NIL:T
	
	Field _data:T[4]
	Field _capacity
	Field _first:Int
	Field _last:Int
	
	Method Grow:Void()
		Local data:=New T[_capacity*2+10]
		If _first<=_last
			For Local i:=_first Until _last
				data[i-_first]=_data[i]
			Next
			_last-=_first
			_first=0
		Else
			Local n:=_capacity-_first
			For Local i:=0 Until n
				data[i]=_data[_first+i]
			Next
			For Local i:=0 Until _last
				data[n+i]=_data[i]
			Next
			_last+=n
			_first=0
		Endif
		_capacity=data.Length
		_data=data
	End

End

Class Enumerator<T> 

	Method New( deque:Deque<T> )
		_deque=deque
	End
	
	Method HasNext:Bool()
		Return _index<_deque.Length-1
	End
	
	Method NextObject:T()
		_index+=1
		Return _deque.Get( _index )
	End
	
	Private
	
	Field _deque:Deque<T>
	Field _index:Int=-1

End

Class IntDeque Extends Deque<Int>

	Method New()
	End
	
	Method New( data:Int[] )
		Super.New( data )
	End

End

Class FloatDeque Extends Deque<Float>

	Method New()
	End
	
	Method New( data:Float[] )
		Super.New( data )
	End

End

Class StringDeque Extends Deque<String>

	Method New()
	End
	
	Method New( data:String[] )
		Super.New( data )
	End

End
