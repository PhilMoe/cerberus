
Import mojo

Import brl.socket

Class UdpEchoServer Implements IOnSendToComplete,IOnReceiveFromComplete

	Method New( port:Int )
		_socket=New Socket( "datagram" )
		If Not _socket.Bind( "",port ) Error "Bind failed"
		_socket.ReceiveFromAsync _data,0,_data.Length,_address,Self
	End
	
	Private
	
	Field _socket:Socket
	Field _data:=New DataBuffer( 1024 )
	Field _address:=New SocketAddress
	
	Method OnSendToComplete:Void( data:DataBuffer,offset:Int,count:Int,address:SocketAddress,source:Socket )
		_socket.ReceiveFromAsync data,0,data.Length,address,Self
	End
	
	Method OnReceiveFromComplete:Void( data:DataBuffer,offset:Int,count:Int,address:SocketAddress,source:Socket )
		_socket.SendToAsync data,offset,count,address,Self
	End
	
End

Class MyApp Extends App Implements IOnConnectComplete,IOnSendComplete,IOnReceiveComplete

	Field _server:UdpEchoServer
	Field _socket:Socket
	Field _data:=New DataBuffer( 1024 )
	
	Field _strs:=New StringList

	Method OnCreate()
	
		_strs.AddLast "Hello"
		_strs.AddLast "World!"
		_strs.AddLast "This is a test"
		_strs.AddLast "Of a"
		_strs.AddLast "UDP EchoServer"

		_server=New UdpEchoServer( 12344 )
		
		_socket=New Socket( "datagram" )
		_socket.ConnectAsync "localhost",12344,self
		
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
		If _strs.IsEmpty
			Print "All done!"
			_socket.Close()
			Return
		Endif
		Local n:=_data.PokeString( 0,_strs.RemoveFirst() )
		_socket.SendAsync _data,0,n,Self
	End
	
	Method OnConnectComplete:Void( connected:Bool,source:Socket )
		If Not connected Error "Connect error"
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