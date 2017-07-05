
Import mojo

Import brl.socket

Class TcpEchoServer Implements IOnAcceptComplete

	Method New( port:Int )
		_socket=New Socket( "server" )
		If Not _socket.Bind( "",port ) Error "Bind failed"
		_socket.AcceptAsync( Self )
	End
	
	Private
	
	Field _socket:Socket
	
	Method OnAcceptComplete:Void( socket:Socket,source:Socket )
		If Not socket Error "Accept error"
		Print "TcpEchoServer: Accepted client connection"
		Local client:=New TcpEchoServerClient( socket )
		_socket.AcceptAsync( Self )
	End
	
End

Class TcpEchoServerClient Implements IOnSendComplete,IOnReceiveComplete

	Method New( socket:Socket )
		_socket=socket
		_socket.ReceiveAsync _data,0,_data.Length,Self
	End
	
	Private
	
	Field _socket:Socket
	Field _data:=New DataBuffer( 1024 )
	
	Method OnReceiveComplete:Void( data:DataBuffer,offset:Int,count:Int,source:Socket )
		If Not count
			Print "TcpEchoServer: Closing client connection"
			_socket.Close()
			Return
		Endif
		_socket.SendAsync data,offset,count,Self
	End

	Method OnSendComplete:Void( data:DataBuffer,offset:Int,count:Int,source:Socket )
		_socket.ReceiveAsync _data,0,_data.Length,Self
	End
	
End

Class MyApp Extends App Implements IOnConnectComplete,IOnSendComplete,IOnReceiveComplete

	Field _server:TcpEchoServer
	Field _socket:Socket
	Field _data:=New DataBuffer( 1024 )
	
	Field _strs:=New StringList

	Method OnCreate()
	
		_strs.AddLast "Hello"
		_strs.AddLast "World!"
		_strs.AddLast "This is a test"
		_strs.AddLast "Of a"
		_strs.AddLast "TCP EchoServer"

		_server=New TcpEchoServer( 12345 )
		
		_socket=New Socket( "stream" )
		_socket.ConnectAsync "localhost",12345,Self
		
		SetUpdateRate 60
	End
	
	Method OnUpdate()
		UpdateAsyncEvents
	End
	
	Method OnRender()
		Cls
		DrawText "Hello World",0,0
	End
	
	Method SendMore:Void()
		If _strs.IsEmpty()
			Print "All done!"
			_socket.Close()
			Return
		Endif
		Local n:=_data.PokeString( 0,_strs.RemoveFirst() )
		_socket.SendAsync _data,0,n,Self
	End
	
	Method OnConnectComplete:Void( connected:Bool,source:Socket )
		If Not connected Error "Error connecting"
		SendMore
	End
	
	Method OnSendComplete:Void( data:DataBuffer,offset:Int,count:Int,source:Socket )
		_socket.ReceiveAsync _data,0,_data.Length,Self
	End

	Method OnReceiveComplete:Void( data:DataBuffer,offset:Int,count:Int,source:Socket )
		Print "Received response:"+data.PeekString( offset,count )
		SendMore
	End

End

Function Main()

	New MyApp
	
End