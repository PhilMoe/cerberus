
Import mojo
Import http

Class MyApp Extends App Implements IOnGetPageComplete

	Field _req:=New HttpRequest
	
	Field _lines:String[]
	
	Method OnCreate()
	
		_req.GetPage "/data/test.txt",Self
		
		SetUpdateRate 60
	End
	
	Method OnUpdate()
		UpdateAsyncEvents
	End
	
	Method OnRender()
		Cls
		For Local i:=0 Until _lines.Length
			DrawText _lines[i],0,i*12
		Next
	End
	
	Method OnGetPageComplete:Void( page:String,source:IAsyncEventSource )
		Print "GetPage complete!"
		_lines=page.Split( "~n" )
	End	
		
End

Function Main()

	New MyApp

End
