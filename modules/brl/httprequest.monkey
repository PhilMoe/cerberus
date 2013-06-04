
#If TARGET<>"html5"

Private

Import brl.socket
Import brl.url

Public

Class HttpRequest Implements IOnConnectComplete,IOnSendComplete,IOnReceiveComplete

	Method New( req:String,url:String,onComplete:IOnHttpRequestComplete )
		Open req,url,onComplete
	End
	
	Method Discard:Void()
		_wbuf.Discard
		_rbuf.Discard
		If _data _data.Discard
		_wbuf=Null
		_rebuf=Null
		_data=Null
	End

	Method Open:Void( req:String,url:String,onComplete:IOnHttpRequestComplete )
		_req=req
		_url=New Url( url,"http",80 )
		_onComplete=onComplete
		_data=Null
		_dataLength=0
		_status=-1
		_response=Null
		_responseText=""
		_bytesReceived=0
		_header=New StringStack
		_header.Push _req+" /"+_url.FullPath()+" HTTP/1.0"
		_header.Push "Host: "+_url.Domain()
	End
	
	Method SetHeader:Void( name:String,value:String )
		_header.Push name+": "+value
	End
	
	Method Send:Void()
		_sock=New Socket( "stream" )
		_sock.ConnectAsync _url.Domain(),_url.Port(),Self
	End
	
	Method Send:Void( data:String,mimeType:String="text/plain;charset=UTF-8",encoding:String="utf8" )
		_data=New DataBuffer( data.Length*3 )
		_dataLength=_data.PokeString( 0,data,encoding )
		If mimeType _header.Push "Content-Type: "+mimeType
		_header.Push "Content-Length: "+_dataLength		
		Send
	End
	
	Method Status:Int()
		Return _status
	End
	
	Method ResponseText:String()
		Return _responseText
	End
	
	Method BytesReceived:Int()
		Return _bytesReceived
	End
	
	Private
	
	Field _wbuf:=New DataBuffer( 4096 )
	Field _rbuf:=New DataBuffer( 16384 )

	Field _req:String
	Field _url:Url
	Field _onComplete:IOnHttpRequestComplete
	Field _header:StringStack
	Field _data:DataBuffer
	Field _dataLength:Int
	Field _sock:Socket
	Field _status:Int
	Field _bytesReceived:Int
	Field _response:StringStack
	Field _responseText:String
	
	Method Finish:Void()
		_sock.Close
		If _response _responseText=_response.Join( "~n" )
		_onComplete.OnHttpRequestComplete Self
	End
	
	Method OnConnectComplete:Void( connected:Bool,source:IAsyncEventSource )
		If Not connected
			Finish
			Return
		Endif

		_header.Push ""
		_header.Push ""
		
		Local t:=_header.Join( "~r~n" )
		Local n:=_wbuf.PokeString( 0,t,"ascii" )
		
		_header.Clear
		_sock.SendAsync _wbuf,0,n,Self
		_sock.ReceiveAsync _rbuf,0,_rbuf.Length,Self
	End
	
	Method OnReceiveComplete:Void( buf:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )

		If Not count
			Finish
			Return
		Endif
		
		_bytesReceived+=count
		
		If _response
			_response.Push buf.PeekString( offset,count,"utf8" )
			_sock.ReceiveAsync( buf,0,buf.Length,Self )
			Return
		Endif
		
		Local start:=0
		Local i:=offset
		Local term:=i+count
		
		While i<term
		
			i+=1
			If buf.PeekByte( i-1 )<>10 Continue
			
			Local t:=buf.PeekString( start,i-start-1,"ascii" ).Trim()
			start=i
			
			If t
				_header.Push t
				Continue
			Endif
			
			If _header.Length
				Local bits:=_header.Get( 0 ).ToUpper().Split( " " )
				If bits.Length>2 And bits[0].StartsWith( "HTTP/" )
					_status=Int( bits[1] )
					_response=New StringStack
					If start<term _response.Push buf.PeekString( start,term-start,"utf8" )
					_sock.ReceiveAsync( buf,0,buf.Length,Self )
					Return
				Endif
			Endif
			
			'ERROR!
			Finish
			Return
			
		Wend
		
		Local n:=term-start
		If start buf.CopyBytes start,buf,0,n
		_sock.ReceiveAsync( buf,n,buf.Length-n,Self )
	End
	
	
	Method OnSendComplete:Void( buf:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
		If Not _dataLength Return
		_sock.SendAsync _data,0,_dataLength,Self
		_dataLength=0
	End

End

#Else If TARGET="html5"

Private

Import brl.thread
Import brl.asyncevent
Import brl.url

Import "native/httprequestthread.${LANG}"

Extern Private

Class BBHttpRequestThread Extends BBThread = "BBHttpRequestThread"
		
	Method Discard:Void()
	
	Method SetHeader:Void(name:String, value:String)
	
	Method Status:Int()
	
	Method ResponseText:String()
	
	Method BytesReceived:Int()
	
	Private
	
	Method Init:Void(req:String, url:String)
	
	Method SendRequest:Void(data:String, mimeType:String)
End

Public

Class HttpRequest Extends BBHttpRequestThread Implements IAsyncEventSource

	Method New(req:String, url:String, onComplete:IOnHttpRequestComplete)
		Open(req, url, onComplete)
	End
	
	Method Open:Void(req:String, url:String, onComplete:IOnHttpRequestComplete)
		_onComplete = onComplete
		Init(req, New Url(url, "http", 80))
	End Method
	
	Method Send:Void()
		SendRequest("", "")
	End Method
	
	Method Send:Void(data:String, mimeType:String = "text/plain;charset=UTF-8", encoding:String = "utf8")
		SendRequest(data, mimeType)
	End Method
	
	Method UpdateAsyncEvents:Void()	
		If IsRunning() Return
		RemoveAsyncEventSource Self
		_onComplete.OnHttpRequestComplete(Self)
	End Method
	
	Private
	
	Field _onComplete:IOnHttpRequestComplete
	
	Method SendRequest:Void(data:String, mimeType:String)
		AddAsyncEventSource Self
		Super.SendRequest(data, mimeType)
	End Method

End Class

#End

Interface IOnHttpRequestComplete

	Method OnHttpRequestComplete:Void( req:HttpRequest )

End
