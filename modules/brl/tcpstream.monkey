
Import brl.stream
Import brl.socket

Class TcpStream Extends Stream

	Method New()
		_socket=New Socket( "stream" )
	End
	
	Method New( socket:Socket )
		If socket.Protocol<>"stream" Error "Socket must be a stream socket"
		_socket=socket
	End
	
	Method Connect:Bool( host:String,port:Int )
		Return _socket.Connect( host,port )
	End
	
	Method Close:Void()
		If Not _socket Return
		_socket.Close
		_socket=Null
		_eof=0
	End
	
	Method Eof:Int() Property
		Return _eof
	End
	
	Method Length:Int() Property
		Return 0
	End
	
	Method Position:Int() Property
		Return 0
	End

	Method Seek:Int( position:Int )
		Return 0
	End
	
	Method Read:Int( data:DataBuffer,offset:Int,count:Int )
		If _eof Or Not _socket Return 0
		Local n:=_socket.Receive( data,offset,count )
		If count And Not n _eof=1
		Return n
	End
	
	Method Write:Int( data:DataBuffer,offset:Int,count:Int )
		If _eof Or Not _socket Return 0
		Local n:=_socket.Send( data,offset,count )
		If count And Not n _eof=1
		Return n
	End
	
	Private
	
	Field _socket:Socket
	Field _eof:Int
	
End
