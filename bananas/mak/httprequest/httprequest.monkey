
Import mojo

Import brl.httprequest

Class MyApp Extends App Implements IOnHttpRequestComplete

	Field get_req:HttpRequest,post_req:HttpRequest
	
	Method OnHttpRequestComplete:Void( req:HttpRequest )
	
		If req=get_req
			Print "Http GET complete!"
		Else
			Print "Http POST complete!"
		Endif

		Print "Status="+req.Status()
		Print "ResponseText="+req.ResponseText()
		
	End
	
	Method OnCreate()
	
		get_req=New HttpRequest( "GET","http://posttestserver.com",Self )
		get_req.Send
		
		post_req=New HttpRequest( "POST","http://posttestserver.com/post.php",Self )
		post_req.Send "Hello World!"
			
		SetUpdateRate 60
	End
	
	Method OnUpdate()

		If KeyHit( KEY_CLOSE ) Error ""

		UpdateAsyncEvents
	End

	Method OnRender()
	
		Cls
		
		DrawText "Http GET bytes received="+get_req.BytesReceived(),0,0
		DrawText "Http POST bytes received="+post_req.BytesReceived(),0,12

	End	
End

Function Main()

	New MyApp

End