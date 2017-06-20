
#If LANG<>"cpp" And LANG<>"java" And LANG<>"cs"
#Error "File streams are not supported on this target"
#Endif

Import mojo

Import brl

Class MyApp Extends App

	Method OnCreate()
	
		SetUpdateRate 60
		
	End
	
	Method OnUpdate()
	
		Local file:=FileStream.Open( "monkey://internal/test_file","w" )
		If Not file Return
		
		For Local i:=0 Until 20
			file.WriteInt i
		Next
		
		file.Close
		
	End
	
	Method OnRender()
	
		Cls
		DrawText "Hello world",0,0
		
		Local file:=FileStream.Open( "monkey://internal/test_file","r" )
		If Not file Return
		
		Local y:=0
		While Not file.Eof()
			y+=12
			Local i:=file.ReadInt()
			DrawText i,0,y
		Wend
		
		file.Close
		
	End
	
End

Function Main()

	New MyApp
	
End
