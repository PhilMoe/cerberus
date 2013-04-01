
Import dom

Class MyApp Extends EventListener

	Field sock:WebSocket

	Method New()
	
		'This is the only server I can find that seems to work!
		sock=createWebSocket( "ws://node.remysharp.com:8001" )
		
		sock.addEventListener "open",Self
		sock.addEventListener "close",Self
		sock.addEventListener "message",Self
		sock.addEventListener "error",Self
		
		Print "Connecting..."
	End
	
	Method handleEvent( ev:Event )
		Select ev.type
		Case "open"
			Print "WebSocket open!"
			sock.send "~qTesting, testing, 1, 2, 3...~q"
		Case "message"
			Print MessageEvent( ev ).data
		End
	End

End

Function Main()

	New MyApp

End

