
#If TARGET<>"glfw" And TARGET<>"stdcpp"
#Error "Invalid target"
#Endif

Import brl.databuffer

Private

Import "native/process.cpp"

Extern

Class BBProcess

	Method Start:Bool( cmd:String )
	Method Kill:Void( retcode:Int )
	Method Wait:Int()

	Method IsRunning:Bool() Property

	Method StdoutAvail:Int() Property
	Method ReadStdout:Int( buf:DataBuffer,offset:Int,count:Int )
	
	Method StderrAvail:Int() Property
	Method ReadStderr:Int( buf:DataBuffer,offset:Int,count:Int )
	
	Method WriteStdin:Int( buf:DataBuffer,offset:Int,count:Int )
	
End

Public

Class Process Extends BBProcess

	Function Execute:String( cmd:String )
	
		Local proc:=New Process
		If Not proc.Start( cmd ) Return ""
		
		Local stdout:=New StringStack
		Local databuf:=New DataBuffer( 1024 )
		
		Repeat
			Local n:=proc.ReadStdout( databuf,0,1024 )
			If n stdout.Push databuf.PeekString( 0,n ) Else Exit
		Forever
		
		Return stdout.Join()
	End
	
End
