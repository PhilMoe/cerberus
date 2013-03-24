
#If TARGET<>"glfw" And TARGET<>"android" And TARGET<>"ios"
#Error "c++ or java Mojo target required"
#Endif

Import mojo

Import brl.asynctcpstream

'***** Hypothetical HTTP module *****

Interface HTTPListener

	Method OnHTTPConnected:Void( getter:HTTPGetter )

	Method OnHTTPDataReceived:Void( data:DataBuffer,offset:Int,count:Int,getter:HTTPGetter )

	Method OnHTTPPageComplete:Void( getter:HTTPGetter )

End

Class HTTPGetter Implements IOnConnectComplete,IOnReadComplete,IOnWriteComplete

	Method GetPage:Void( host:String,port:Int,listener:HTTPListener )
		_host=host
		_port=port
		_listener=listener
		
		_stream=New AsyncTcpStream
		
		_stream.Connect _host,_port,Self
	End
	
	Method Update:Bool()
		If _stream Return _stream.Update()
		Return False
	End
	
	Private
	
	Method Finish:Void()
		_listener.OnHTTPPageComplete Self
		_strqueue.Clear
		_stream.Close
		_stream=Null
	
	End
	
	'start up another read op
	Method ReadMore:Void()
		'read another block
		_stream.ReadAll _rbuf,0,_rbuf.Length,Self
	End

	'start up another write op
	Method WriteMore:Void()
	
		If _strqueue.IsEmpty() Return
		
		Local str:=_strqueue.RemoveFirst()
		
		_wbuf.PokeString 0,str
		
		_stream.WriteAll _wbuf,0,str.Length,Self
	End
	
	Method WriteString:Void( str:String )
	
		_strqueue.AddLast str
		
	End

	Method OnConnectComplete:Void( connected:Bool,source:IAsyncEventSource )
	
		If Not connected
			Finish
			Return
		Endif

		WriteString "GET / HTTP/1.0~r~n"
		WriteString "Host: "+_host+"~r~n"
		WriteString "~r~n"
		
		_listener.OnHTTPConnected Self

		WriteMore
		
		ReadMore
	End

	Method OnReadComplete:Void( buf:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
	
		If Not count	'EOF!
			Finish
			Return
		Endif
		
		_listener.OnHTTPDataReceived buf,offset,count,Self
		
		ReadMore
	End
	
	Method OnWriteComplete:Void( buf:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
	
		WriteMore
	End
	
	Field _host:String
	Field _port:Int	
	Field _listener:HTTPListener
	
	Field _stream:AsyncTcpStream
	Field _strqueue:=New StringList
	Field _rbuf:=New DataBuffer( 1024 )	'thrash it!
	Field _wbuf:=New DataBuffer( 256 )
	
End

'***** The app! *****

Class MyApp Extends App Implements HTTPListener

	Field sx:Int,sy:Int
	Field mx:Int,my:Int

	Field lines:=New StringStack

	Field getter:=New HTTPGetter

	Method OnHTTPConnected:Void( getter:HTTPGetter )
	
		Print "HTTP connected!"
		
	End
	
	Method OnHTTPDataReceived:Void( data:DataBuffer,offset:Int,count:Int,getter:HTTPGetter )
	
		Print "HTTP data received! length="+count
		
		Local str:=data.PeekString( offset,count )
		
		For Local line:=Eachin str.Split( "~n" )
			If line.Length>80
				While line.Length>80
					lines.Push line[..80]
					line=line[80..]
				Wend
				If Not line Continue
			Endif
			lines.Push line
		Next
	End
	
	Method OnHTTPPageComplete:Void( getter:HTTPGetter )

		Print "HTTP page complete!"
		
	End
	
	Method OnCreate()
	
		getter.GetPage "www.monkeycoder.co.nz",80,Self
		
		SetUpdateRate 60

	End
	
	Method OnUpdate()
	
		UpdateAsyncEvents
		
		If MouseDown( 0 ) And Not MouseHit( 0 )
			sx-=MouseX-mx
			sy-=MouseY-my
		Endif
		mx=MouseX
		my=MouseY

	End

	Method OnRender()
	
		Cls
		
		Translate 0,-sy
		
		DrawText "Hello world",0,0
		
		For Local i:=0 Until lines.Length
		
			Local y:=i*12+12
		
			If y<sy-FontHeight() Or y>=sy+DeviceHeight() Continue
			
			DrawText lines.Get( i ),0,y
		Next
	End	
End

Function Main()
	New MyApp
End
