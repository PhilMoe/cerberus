
#If TARGET<>"stdcpp" And TARGET<>"glfw"
#Error "TcpServer is only available for stdcpp and glfw targets"
#Endif

Import tcpstream
Import "native/tcpserver.cpp"

Extern Private

Class BBTcpServer

	Method Create:Bool( port:Int )
	
	Method Port:Int()
	
	Method Listen:Void( backlog:Int )
	
	Method Accept:BBTcpStream()
	
	Method Close:Void()
	
End

Public

Class TcpServer

	Method Port:Int() Property
		Return _server.Port()
	End

	Method Listen:Void( backlog:Int )
		_server.Listen( backlog )
	End
	
	Method Accept:TcpStream()
		Local stream:=_server.Accept()
		If stream Return New TcpStream( stream )
		Return Null
	End
	
	Method Close:Void()
		_server.Close()
		_server=Null
	End
	
	Function Create:TcpServer( port:Int=0 )
		Local server:=New BBTcpServer
		If server.Create( port ) Return New TcpServer( server )
		Return Null
	End
	
	Private
	
	Field _server:BBTcpServer
	
	Method New()
	End
	
	Method New( server:BBTcpServer )
		_server=server
	End
	
End
