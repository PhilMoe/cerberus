
Private

Import brl.databuffer

Public

Class RingBuffer

	Method New( length:Int,dynamic:Bool )
		_buf=New DataBuffer( length )
		_dynamic=dynamic
	End
	
	Method Clear:Void()
		_get=0
		_put=0
		_canget=0
	End
	
	Method IsDynamic:Bool()
		Return _dynamic
	End

	Method IsEmpty:Bool()
		Return _canget=0
	End
	
	Method IsFull:Bool()
		Return _canget=_buf.Length
	End
	
	Method Capacity:Int()
		Return _buf.Length
	End
	
	Method CanPut:Int()
		Return _buf.Length-_canget
	End

	Method CanGet:Int()
		Return _canget
	End
	
	Method Put:Int( src:DataBuffer,offset:Int,count:Int )
	
		Local canput:=_buf.Length-_canget
		
		If count>canput
			If _dynamic
				'grow ring buffer - yikes!
				Local buf:=New DataBuffer( Max( _buf.Length*2+_buf.Length/2,_canget+count ) )
				Local off:=0,cnt:=_canget,get=_get
				Local n:=_buf.Length-get
				If cnt>n
					_buf.CopyBytes get,buf,off,n
					off+=n
					cnt-=n
					get=0
				Endif
				_buf.CopyBytes get,buf,off,cnt
				_buf.Discard
				_buf=buf
			Else
				count=canput
			Endif
		Endif
		
		'copy to ring buffer
		Local off:=offset,cnt:=count
		Local n:=_buf.Length-_put
		If count>=n
			src.CopyBytes off,_buf,_put,n
			off+=n
			cnt-=n
			_put=0
			_canget+=n
		Endif
		src.CopyBytes off,_buf,_put,cnt
		_put+=cnt
		_canget+=cnt
		
		Return count
	End
	
	Method Peek:Int( dst:DataBuffer,offset:Int,count:Int )

		count=Min( count,_canget )

		'peek from ring buffer
		Local off:=offset,cnt:=count,get:=_get
		Local n:=_buf.Length-get
		If cnt>=n
			_buf.CopyBytes get,dst,off,n
			off+=n
			cnt-=n
			get=0
		Endif
		_buf.CopyBytes get,dst,off,cnt
		
		Return count
	End
	
	Method Get:Int( dst:DataBuffer,offset:Int,count:Int )
	
		count=Min( count,_canget )
		
		'copy from ring buffer
		Local off:=offset,cnt:=count
		Local n:=_buf.Length-_get
		If cnt>=n
			_buf.CopyBytes _get,dst,off,n
			off+=n
			cnt-=n
			_get=0
			_canget-=n
		Endif
		_buf.CopyBytes _get,dst,off,cnt
		_get+=cnt
		_canget-=cnt
		
		Return count
	End
	
	'low level accesss.
	Method GetDataBuffer:DataBuffer()
		Return _buf
	End
	
	Method GetGetCursor:Int()
		Return _get
	End
	
	Method GetPutCursor:Int()
		Return _put
	End
	
	Private
	
	Field _buf:DataBuffer
	Field _dynamic:Bool
	Field _get:Int,_put:Int,_canget:Int

End
