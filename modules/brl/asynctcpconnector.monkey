
#If LANG<>"cpp" And LANG<>"java"
#Error "tcp streams are unavailable on this target"
#Endif

Import brl.tcpstream
Import brl.asyncevent

Private

Import brl.thread

Public

Interface IOnConnectComplete
	Method OnConnectComplete:Void( connected:Bool,source:IAsyncEventSource )
End

Class AsyncTcpConnector Extends Thread Implements IAsyncEventSource

	Method Connect:Void( stream:TcpStream,host:String,port:Int,onComplete:IOnConnectComplete )
		AddAsyncEventSource Self
		_stream=stream.GetBBTcpStream()
		_onComplete=onComplete
		_host=host
		_port=port
		Start
	End

	Private
	
	Field _stream:BBTcpStream
	Field _onComplete:IOnConnectComplete
	Field _host:String
	Field _port:Int
	Field _connected:Bool
	
	Method Run__UNSAFE__:Void()
		_connected=_stream.Connect( _host,_port )
	End
	
	Method UpdateAsyncEvents:Void()
		If IsRunning() Return
		RemoveAsyncEventSource Self
		_onComplete.OnConnectComplete _connected,Self
	End
End
