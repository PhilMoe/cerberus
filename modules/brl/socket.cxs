
Import brl.databuffer
Import brl.asyncevent

#If Not BRL_SOCKET_IMPLEMENTED

#If TARGET="glfw" Or TARGET="stdcpp"

#BRL_SOCKET_IMPLEMENTED=True
Import "native/socket.cpp"

#Else If TARGET="android"

#BRL_SOCKET_IMPLEMENTED=True
Import "native/socket.java"

#Else If TARGET="winrt"

#BRL_SOCKET_IMPLEMENTED=True
Import "native/socket_winrt.cpp"

#Else If TARGET="ios"

#BRL_SOCKET_IMPLEMENTED=True
Import "native/socket_ipv6.cpp"

#Endif

#Endif

#If Not BRL_SOCKET_IMPLEMENTED

#Error "Native Socket class not implemented."

#Endif

Private

Import brl.thread

Extern Private

Class BBSocketAddress

	Method Set:Void( host:String,port:Int )
	Method Set:Void( address:BBSocketAddress )
	Method Host:String()
	Method Port:Int()

End

Class BBSocket

	Method Open:Bool( protocol:Int )
	Method Close:Void()
	
	Method Bind:Bool( host:String,port:Int )
	Method Connect:Bool( host:String,port:Int )
	Method Listen:Bool( backlog:Int )
	Method Accept:Bool()
	Method Accepted:BBSocket()
	
	Method Send:Int( buf:BBDataBuffer,offset:Int,count:Int )
	Method Receive:Int( buf:BBDataBuffer,offset:Int,count:Int )

	Method SendTo:Int( buf:BBDataBuffer,offset:Int,count:Int,address:BBSocketAddress )
	Method ReceiveFrom:Int( buf:BBDataBuffer,offset:Int,count:Int,address:BBSocketAddress )

	Method GetLocalAddress:Void( address:BBSocketAddress )
	Method GetRemoteAddress:Void( address:BBSocketAddress )
	
End

Private

Class AsyncOp

	'Careful...called by background thread
	Method Execute__UNSAFE__:Void( source:Socket ) Abstract

	'Relax...called by main thread	
	Method Complete:Void( source:Socket ) Abstract

End

Class AsyncQueue Extends Thread Implements IAsyncEventSource

	Method New( source:Socket )
		Self.source=source
	End
	
	Method IsBusy:Bool() Property
		Return get<>put
	End
	
	Method Enqueue:Void( op:AsyncOp )
		queue[put]=op
		put=(put+1) Mod QUEUE_SIZE
		If put=get Error "AsyncQueue queue overflow!"
		Start						'NOP if already running. Race condition alert! This will fail if thread is in the process of exiting!
	End
	
	Method UpdateAsyncEvents:Void()
		If nxt<>put
'			If Not IsRunning() Print "RACE!"
			Start					'NOP if already running. This is a kludge for the above race condition...
		Endif			
		While get<>nxt
			Local op:=queue[get]
			get=(get+1) Mod QUEUE_SIZE
			op.Complete source
		Wend
	End
	
	Private
	
	Method Run__UNSAFE__:Void()
		While nxt<>put
			queue[nxt].Execute__UNSAFE__ source
			nxt=(nxt+1) Mod QUEUE_SIZE
		Wend
	End
	
	Private

	Const QUEUE_SIZE:=256			'how many ops can be queued. Overflow this and yer hosed.
	Const QUEUE_MASK:=QUEUE_SIZE-1

	Field source:Socket
	Field queue:AsyncOp[QUEUE_SIZE]
	Field put:Int	'only written by Enqueue
	Field get:Int	'only written by Update
	Field nxt:Int	'only written by thread
	
End

Class AsyncConnectOp Extends AsyncOp

	Method New( socket:BBSocket,host:String,port:Int,onComplete:IOnConnectComplete )
		_socket=socket
		_host=host
		_port=port
		_onComplete=onComplete
	End
	
	Private
	
	Field _socket:BBSocket
	Field _host:String
	Field _port:Int
	Field _onComplete:IOnConnectComplete
	Field _result:Bool

	Method Execute__UNSAFE__:Void( source:Socket )
		_result=_socket.Connect( Thread.Strdup(_host),_port )
	End
	
	Method Complete:Void( source:Socket )
		If _result Socket( source ).OnConnectComplete()
		_onComplete.OnConnectComplete( _result,source )
	End
	
End

Class AsyncBindOp Extends AsyncOp

	Method New( socket:BBSocket,host:String,port:Int,onComplete:IOnBindComplete )
		_socket=socket
		_host=host
		_port=port
		_onComplete=onComplete
	End
	
	Private
	
	Field _socket:BBSocket
	Field _host:String
	Field _port:Int
	Field _onComplete:IOnBindComplete
	Field _result:Bool

	Method Execute__UNSAFE__:Void( source:Socket )
		_result=_socket.Bind( Thread.Strdup(_host),_port )
	End
	
	Method Complete:Void( source:Socket )
		If _result Socket( source ).OnBindComplete()
		_onComplete.OnBindComplete( _result,source )
	End
	
End

Class AsyncAcceptOp Extends AsyncOp

	Method New( socket:BBSocket,onComplete:IOnAcceptComplete )
		_socket=socket
		_onComplete=onComplete
	End
	
	Private
	
	Field _socket:BBSocket
	Field _onComplete:IOnAcceptComplete
	Field _result:Bool
	
	Method Execute__UNSAFE__:Void( source:Socket )
		_result=_socket.Accept()
	End
	
	Method Complete:Void( source:Socket )
		Local sock:Socket
		If _result sock=Socket( source ).OnAcceptComplete()
		_onComplete.OnAcceptComplete( sock,source )
	End
End

Class AsyncSocketIoOp Extends AsyncOp

	Method New( socket:BBSocket,data:DataBuffer,offset:Int,count:Int )
		_socket=socket
		_data=data
		_offset=offset
		_count=count
	End
	
	Private
	
	Field _socket:BBSocket
	Field _data:DataBuffer
	Field _offset:Int
	Field _count:Int

End

Class AsyncSendOp Extends AsyncSocketIoOp

	Method New( socket:BBSocket,data:DataBuffer,offset:Int,count:Int,onComplete:IOnSendComplete )
		Super.New( socket,data,offset,count )
		_onComplete=onComplete
	End
	
	Private
	
	Field _onComplete:IOnSendComplete
	
	Method Execute__UNSAFE__:Void( source:Socket )
		_count=_socket.Send( _data,_offset,_count )
	End
	
	Method Complete:Void( source:Socket )
		_onComplete.OnSendComplete( _data,_offset,_count,source )
	End

End

Class AsyncSendToOp Extends AsyncSocketIoOp

	Method New( socket:BBSocket,data:DataBuffer,offset:Int,count:Int,address:SocketAddress,onComplete:IOnSendToComplete )
		Super.New( socket,data,offset,count )
		_address=address
		_onComplete=onComplete
	End
	
	Private
	
	Field _address:SocketAddress
	Field _onComplete:IOnSendToComplete
	
	Method Execute__UNSAFE__:Void( source:Socket )
		_count=_socket.SendTo( _data,_offset,_count,_address )
	End
	
	Method Complete:Void( source:Socket )
		_onComplete.OnSendToComplete( _data,_offset,_count,_address,source )
	End

End

Class AsyncReceiveOp Extends AsyncSocketIoOp

	Method New( socket:BBSocket,data:DataBuffer,offset:Int,count:Int,onComplete:IOnReceiveComplete )
		Super.New( socket,data,offset,count )
		_onComplete=onComplete
	End
	
	Private
	
	Field _onComplete:IOnReceiveComplete
	
	Method Execute__UNSAFE__:Void( source:Socket )
		_count=_socket.Receive( _data,_offset,_count )
	End
	
	Method Complete:Void( source:Socket )
		_onComplete.OnReceiveComplete( _data,_offset,_count,source )
	End
End

Class AsyncReceiveAllOp Extends AsyncSocketIoOp

	Method New( socket:BBSocket,data:DataBuffer,offset:Int,count:Int,onComplete:IOnReceiveComplete )
		Super.New( socket,data,offset,count )
		_onComplete=onComplete
	End
	
	Private
	
	Field _onComplete:IOnReceiveComplete
	
	Method Execute__UNSAFE__:Void( source:Socket )
		Local i:=0
		While i<_count
			Local n:=_socket.Receive( _data,_offset+i,_count-i )
			If n>0 i+=n Else Exit
		Wend
		_count=i
	End
	
	Method Complete:Void( source:Socket )
		_onComplete.OnReceiveComplete( _data,_offset,_count,source )
	End
End

Class AsyncReceiveFromOp Extends AsyncSocketIoOp

	Method New( socket:BBSocket,data:DataBuffer,offset:Int,count:Int,address:SocketAddress,onComplete:IOnReceiveFromComplete )
		Super.New( socket,data,offset,count )
		_address=address
		_onComplete=onComplete
	End
	
	Private
	
	Field _address:SocketAddress
	Field _onComplete:IOnReceiveFromComplete
	
	Method Execute__UNSAFE__:Void( source:Socket )
		_count=_socket.ReceiveFrom( _data,_offset,_count,_address )
	End
	
	Method Complete:Void( source:Socket )
		_onComplete.OnReceiveFromComplete( _data,_offset,_count,_address,source )
	End
End

Public

Interface IOnConnectComplete
	Method OnConnectComplete:Void( connected:Bool,source:Socket )
End

Interface IOnBindComplete
	Method OnBindComplete:Void( bound:Bool,source:Socket )
End

Interface IOnAcceptComplete
	Method OnAcceptComplete:Void( socket:Socket,source:Socket )
End

Interface IOnSendComplete
	Method OnSendComplete:Void( data:DataBuffer,offset:Int,count:Int,source:Socket )
End

Interface IOnSendToComplete
	Method OnSendToComplete:Void( data:DataBuffer,offset:Int,count:Int,address:SocketAddress,source:Socket )
End

Interface IOnReceiveComplete
	Method OnReceiveComplete:Void( data:DataBuffer,offset:Int,count:Int,source:Socket )
End

Interface IOnReceiveFromComplete
	Method OnReceiveFromComplete:Void( data:DataBuffer,offset:Int,count:Int,address:SocketAddress,source:Socket )
End

Class SocketAddress Extends BBSocketAddress

	Method New()
	End
	
	Method New( host:String,port:Int )
		Set( host,port )
	End
	
	Method New( address:SocketAddress )
		Set( address )
	End
	
	Method Host:String() Property
		Return Super.Host()
	End
	
	Method Port:Int() Property
		Return Super.Port()
	End
	
	Method ToString:String() Property
		Return Host+":"+Port
	End
End

Class Socket Implements IAsyncEventSource

	Method New( protocol:String="stream" )
		Local proto:Int
		Select protocol
		Case "stream" proto=STREAM
		Case "server" proto=SERVER
		Case "datagram" proto=DATAGRAM
		Default Error "Illegal socket protocol"
		End
		_sock=New BBSocket
		If Not _sock.Open( proto ) Error "Socket open failed"
		_proto=proto
		_state=OPEN
		Start()
	End
	
	Method Close:Void()
		_sock.Close()
		_state=0
	End
	
	Method Bind:Bool( host:String,port:Int )
		If Not IsOpen Or IsBound Return False
		If Not _sock.Bind( host,port ) Return False
		OnBindComplete()
		Return True
	End
	
	Method BindAsync:Void( host:String,port:Int,onComplete:IOnBindComplete )
		If Not IsOpen Or IsBound Return
		_rthread.Enqueue New AsyncBindOp( _sock,host,port,onComplete )
	End
	
	Method Connect:Bool( host:String,port:Int )
		If Not IsOpen Or IsConnected Or IsListening Return False
		If Not _sock.Connect( host,port ) Return False
		OnConnectComplete()
		Return True
	End
	
	Method ConnectAsync:Void( host:String,port:Int,onComplete:IOnConnectComplete )
		If Not IsOpen Or IsConnected Or IsListening Return
		_rthread.Enqueue New AsyncConnectOp( _sock,host,port,onComplete )
	End
	
	Method Accept:Socket()
		If Not IsListening And (Not IsOpen Or _proto<>SERVER Or Not Bind( "",0 )) Return Null
		If Not _sock.Accept() Return Null
		Return OnAcceptComplete()
	End
	
	Method AcceptAsync:Void( onComplete:IOnAcceptComplete )
		If Not IsListening Return
		_rthread.Enqueue New AsyncAcceptOp( _sock,onComplete )
	End
	
	Method Send:Int( buf:DataBuffer,offset:Int,count:Int )
		If Not IsConnected Return 0
		Local n:=_sock.Send( buf,offset,count )
		If n>=0 Return n
		Return 0
	End
		
	Method SendAsync:Void( buf:DataBuffer,offset:Int,count:Int,onComplete:IOnSendComplete )
		If Not IsConnected Return
		_wthread.Enqueue New AsyncSendOp( _sock,buf,offset,count,onComplete )
	End
	
	Method SendTo:Int( buf:DataBuffer,offset:Int,count:Int,address:SocketAddress )
		Local n:=_sock.SendTo( buf,offset,count,address )
		If n>=0 Return n
		Return 0
	End
	
	Method SendToAsync:Int( buf:DataBuffer,offset:Int,count:Int,address:SocketAddress,onComplete:IOnSendToComplete )
		If _proto<>DATAGRAM Or IsConnected Return
		_wthread.Enqueue New AsyncSendToOp( _sock,buf,offset,count,address,onComplete )
	End
	
	Method Receive:Int( buf:DataBuffer,offset:Int,count:Int )
		If Not IsConnected Return 0
		Local n:=_sock.Receive( buf,offset,count )
		If n>=0 Return n
		Return 0
	End

	Method ReceiveAsync:Void( buf:DataBuffer,offset:Int,count:Int,onComplete:IOnReceiveComplete )
		If Not IsConnected Return
		_rthread.Enqueue New AsyncReceiveOp( _sock,buf,offset,count,onComplete )
	End
	
	Method ReceiveAll:Int( buf:DataBuffer,offset:Int,count:Int )
		If Not IsConnected Return 0
		Local i:=0
		While i<count
			Local n:=_sock.Receive( buf,offset+i,count-i )
			If n>0 i+=n Else Exit
		Wend
		Return i
	End

	Method ReceiveAllAsync:Void( buf:DataBuffer,offset:Int,count:Int,onComplete:IOnReceiveComplete )
		If Not IsConnected Return
		_rthread.Enqueue New AsyncReceiveAllOp( _sock,buf,offset,count,onComplete )
	End
	
	Method ReceiveFrom:Int( buf:DataBuffer,offset:Int,count:Int,address:SocketAddress )
		If _proto<>DATAGRAM Or IsConnected Return 0
		Local n:=_sock.ReceiveFrom( buf,offset,count,address )
		If n>=0 Return n
		Return 0
	End
	
	Method ReceiveFromAsync:Int( buf:DataBuffer,offset:Int,count:Int,address:SocketAddress,onComplete:IOnReceiveFromComplete )
		If _proto<>DATAGRAM Or IsConnected Return
		_rthread.Enqueue New AsyncReceiveFromOp( _sock,buf,offset,count,address,onComplete )
	End
	
	Method IsOpen:Bool() Property
		Return (_state & OPEN)<>0
	End
	
	Method IsBound:Bool() Property
		Return (_state & BOUND)<>0
	End
	
	Method IsConnected:Bool() Property
		Return (_state & CONNECTED)<>0
	End
	
	Method IsListening:Bool() Property
		Return (_state & LISTENING)<>0
	End
	
	Method Protocol:String() Property
		Select _proto
		Case STREAM Return "stream"
		Case SERVER Return "server"
		Case DATAGRAM Return "datagram"
		End
		Return "?"
	End
	
	Method LocalAddress:SocketAddress() Property
		_sock.GetLocalAddress( _localAddress )
		Return _localAddress
	End
	
	Method RemoteAddress:SocketAddress() Property
		_sock.GetRemoteAddress( _remoteAddress )
		Return _remoteAddress
	End
	
	Private
	
	Const STREAM:=1
	Const SERVER:=2
	Const DATAGRAM:=3

	Const OPEN:=1
	Const BOUND:=2
	Const CONNECTED:=4
	Const LISTENING:=8
	
	Field _sock:BBSocket
	Field _proto:Int
	Field _state:Int
	Field _rthread:AsyncQueue
	Field _wthread:AsyncQueue
	Field _localAddress:=New SocketAddress
	Field _remoteAddress:=New SocketAddress
	
	Method New( sock:BBSocket,proto:Int,state:Int )
		_sock=sock
		_proto=proto
		_state=state
		Start()
	End
	
	Method Start:Void()
		_rthread=New AsyncQueue( Self )
		_wthread=New AsyncQueue( Self )
		AddAsyncEventSource Self
	End
	
	Method UpdateAsyncEvents:Void()
		If _rthread _rthread.UpdateAsyncEvents
		If _wthread	_wthread.UpdateAsyncEvents
	End
	
	Method OnConnectComplete:Void()
		_state|=Socket.CONNECTED|Socket.BOUND
	End
		
	Method OnBindComplete:Void()
		_state|=BOUND
		If _proto=SERVER
			_sock.Listen( 1 )
			_state|=LISTENING
		Endif
	End
	
	Method OnAcceptComplete:Socket()
		Return New Socket( _sock.Accepted(),Socket.STREAM,Socket.OPEN|Socket.BOUND|Socket.CONNECTED )
	End
		
End
