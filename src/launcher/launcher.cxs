
#If HOST<>"linux"
#Error "Host<>linux"
#Endif

#If TARGET<>"stdcpp"
#Error "Target<>stdcpp"
#Endif

Import brl.process
Import brl.filepath

Function Main()

	Local args:=""
	
	For Local i:=1 Until AppArgs.Length
		Local arg:=AppArgs[i]
		If arg.Contains( " " ) arg="~q"+arg+"~q"
		args+=" "+arg
	Next
	
	Local dir:=ExtractDir( AppPath() )
	
#If HOST="macos"

	Local cmd:=dir+"/bin/Ted.app"
	If args cmd="open -n "+cmd+" --args"+args Else cmd="open "+cmd
	Execute cmd
	Sleep 100
	
#Else If HOST="linux"

	Execute dir+"/bin/Ted"+args+" >/dev/null 2>/dev/null &"

#Endif

End
