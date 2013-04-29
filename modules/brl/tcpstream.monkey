
Import brl.stream

#If (LANG="cpp" Or LANG="java")
#BRL_TCPSTREAM_IMPLEMENTED=True
Import "native/tcpstream.${LANG}"
#Endif

#BRL_TCPSTREAM_IMPLEMENTED=False
#If BRL_TCPSTREAM_IMPLEMENTED="0"
#Error "Native TcpStream class not found."
#Endif

Extern

Class BBTcpStream Extends BBStream

	Method Connect:Bool( host:String,port:Int )
	Method ReadAvail:Int()
	Method WriteAvail:Int()

End

Public

Class TcpStream Extends Stream

	Method New()
		_stream=New BBTcpStream
	End

	Method Connect:Bool( host:String,port:Int )
		Return _stream.Connect( host,port )
	End
	
	Method ReadAvail:Int()
		Return _stream.ReadAvail()
	End
	
	Method WriteAvail:Int()
		Return _stream.WriteAvail()
	End
	
	'Stream
	Method Close:Void()
		If _stream 
			_stream.Close
			_stream=Null
		Endif
	End
	
	Method Eof:Int()
		Return _stream.Eof()
	End
	
	Method Length:Int()
		Return _stream.Length()
	End
	
	Method Position:Int()
		Return _stream.Position()
	End
	
	Method Seek:Int( position:Int )
		Return _stream.Seek( position )
	End
	
	Method Read:Int( buffer:DataBuffer,offset:Int,count:Int )
		Return _stream.Read( buffer,offset,count )
	End
	
	Method Write:Int( buffer:DataBuffer,offset:Int,count:Int )
		Return _stream.Write( buffer,offset,count )
	End
	
	'***** INTERNAL *****
	Method GetBBTcpStream:BBTcpStream()
		Return _stream
	End
	
	Private
	
	Field _stream:BBTcpStream
	
End
