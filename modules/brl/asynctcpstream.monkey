
Import brl.asyncstream
Import brl.asynctcpconnector

Class AsyncTcpStream Extends AsyncStream Implements IOnConnectComplete

	Method Connect:Void( host:String,port:Int,onComplete:IOnConnectComplete )
		If _tcpStream Error "Already connected"
		_onConnect=onComplete
		_tcpStream=New TcpStream
		(New AsyncTcpConnector).Connect _tcpStream,host,port,Self
	End
	
	Private
	
	Field _tcpStream:TcpStream
	Field _onConnect:IOnConnectComplete
	
	Method OnConnectComplete:Void( connected:Bool,source:IAsyncEventSource )
		If connected Super.Start _tcpStream.GetBBTcpStream()
		_onConnect.OnConnectComplete connected,Self
	End

End
