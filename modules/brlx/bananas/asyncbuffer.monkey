
Import brl.stream

Import brl.asyncstream

Private

Import brl.ringbuffer

Public

Class AsyncBuffer Extends Stream Implements IOnReadComplete,IOnWriteComplete

	Method New( stream:AsyncStream )
		_stream=stream
		ReadMore
	End

	Method Flush()
		WriteMore
	End
	
	Method ReadAvail:Int()
		Return _rqueue.CanGet()
	End
	
	Method WriteAvail:Int()
		Return _wqueue.CanPut()
	End
	
	'***** Stream methods *****
	
	Method Eof:Int()
		Return _eof
	End
	
	Method Close:Void()
	End
	
	Method Length:Int()
		Return 0
	End
	
	Method Position:Int()
		Return 0
	End
	
	Method Seek:Int( position:Int )
		Return 0
	End
	
	Method Read:Int( buffer:DataBuffer,offset:Int,count:Int )
		Return _rqueue.Get( buffer,offset,count )
	End
	
	Method Write:Int( buffer:DataBuffer,offset:Int,count:Int )
		Return _wqueue.Put( buffer,offset,count )
	End
	
	Private
	
	Field _stream:AsyncStream
	
	Field _rbuf:=New DataBuffer( 1024 )
	Field _rqueue:=New RingBuffer( 4096,True )
	
	Field _wbuf:=New DataBuffer( 1024 )
	Field _wqueue:=New RingBuffer( 4096,True )
	
	Field _eof:Bool
	
	Method ReadMore:Void()
		_stream.Read _rbuf,0,_rbuf.Length,Self
	End
	
	Method WriteMore:Void()
		If Not _wqueue.IsEmpty() _stream.Write _wbuf,0,_wqueue.Get( _wbuf,0,_wbuf.Length ),Self
	End
	
	Method OnReadComplete:Void( data:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
		Print "OnReadComplete, count="+count
		If count
			_rqueue.Put data,offset,count
			ReadMore
		Else
			_eof=True
		Endif
	End
	
	Method OnWriteComplete:Void( data:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
		Print "OnWriteComplete, count="+count
		If count
			WriteMore
		Else 
			Error "PANIC!"
		Endif
	End
	
End
