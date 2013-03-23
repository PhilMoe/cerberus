
Import brl.databuffer

Interface SocketListener

	Method OnSocketOpen:Void( socket:Socket )
	
	Method OnSocketMessage:Void( msg:DataBuffer,offset:Int,count:Int,socket:Socket )
	
	Method OnSocketError:Void( socket:Socket )
	
End

Class Socket

	Method New( listener:SocketListener )

	Method Open:Void( host:String )
	
	Method SendMessage:Void( msg:DataBuffer,offset:Int,count:Int )
	
	Method Close:Void()

End
