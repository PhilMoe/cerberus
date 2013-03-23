
Import mojo

Import brl.asynctcpstream

Import asyncbuffer

Class MyApp Extends App Implements IOnConnectComplete

	Field _stream:AsyncTcpStream
	Field _buffer:AsyncBuffer

	Method OnConnectComplete:Void( connected:Bool,source:IAsyncEventSource )
	
		Print "OnConnectComplete, connected="+Int(connected)
	
		_buffer=New AsyncBuffer( _stream )
		
		_buffer.WriteLine "GET / HTTP/1.0"
		_buffer.WriteLine "Host: www.monkeycoder.co.nz"
		_buffer.WriteLine ""
		
		_buffer.Flush
		
	End
	
	Method OnCreate()
	
		_stream=New AsyncTcpStream
		
		_stream.Connect "www.monkeycoder.co.nz",80,Self
		
		SetUpdateRate 60
	
	End
	
	Method OnUpdate()

		UpdateAsyncEvents

	End
	
	Method OnRender()
	
		Cls
		
		If _buffer DrawText _buffer.ReadAvail(),0,0
		
	End		

End

Function Main()
	New MyApp
End
