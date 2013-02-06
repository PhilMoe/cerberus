
Import brl.stream

Class DataStream Extends Stream

	Method New( buf:DataBuffer,offset:Int=0 )
		_buffer=buf
		_offset=offset
		_length=buf.Length-offset
	End
	
	Method New( buf:DataBuffer,offset:Int,length:Int )
		_buffer=buf
		_offset=offset
		_length=length
	End
	
	Method Length:Int() Property
		Return _length
	End
	
	Method Position:Int() Property
		Return _position
	End
	
	Method Seek:Int( position:Int )
		_position=Clamp( position,0,_length-1 )
		Return _position
	End

	Method Eof:Int()
		Return _position=_length
	End
	
	Method Close:Void()
		If _buffer
			_buffer=Null
			_position=0
			_length=0
		Endif
	End
	
	Method Read:Int( buf:DataBuffer,offset:Int,count:Int )
		If _position+count>_length count=_length-_position
		For Local i:=0 Until count
			buf.PokeByte offset+i,_buffer.PeekByte( _offset+_position+i )
		Next
		_position+=count
		Return count
	End
	
	Method Write:Int( buf:DataBuffer,offset:Int,count:Int )
		If _position+count>_length count=_length-_position
		For Local i:=0 Until count
			_buffer.PokeByte _offset+_position+i,buf.PeekByte( offset+i )
		Next
		_position+=count
		Return count
	End
	
	Private
	
	Field _buffer:DataBuffer
	Field _position:Int
	Field _offset:Int
	Field _length:Int
	
End
