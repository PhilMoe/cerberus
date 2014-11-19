
Import brl.stream

Class DataStream Extends Stream

	Method New( buffer:DataBuffer,offset:Int=0 )
		_buffer=buffer
		_offset=offset
		_length=buffer.Length-offset
	End
	
	Method New( buffer:DataBuffer,offset:Int,length:Int )
		_buffer=buffer
		_offset=offset
		_length=length
	End
	
	Method Data:DataBuffer() Property
		Return _buffer
	End
	
	Method Offset:Int() Property
		Return _offset
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
			_offset=0
			_length=0
			_position=0
		Endif
	End
	
	Method Read:Int( buf:DataBuffer,offset:Int,count:Int )
		If _position+count>_length count=_length-_position

		_buffer.CopyBytes _offset+_position,buf,offset,count
		_position+=count

		Return count
	End
	
	Method Write:Int( buf:DataBuffer,offset:Int,count:Int )
		If _position+count>_length count=_length-_position
		
		buf.CopyBytes offset,_buffer,_offset+_position,count
		_position+=count
		
		Return count
	End
	
	Private
	
	Field _buffer:DataBuffer
	Field _offset:Int
	Field _length:Int
	Field _position:Int
	
End
