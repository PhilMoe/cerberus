
Import stream

#If LANG="cpp"
Import "native/filestream.cpp"
#Endif

Extern

Class BBFileStream Extends BBStream

	'FileStream methods...
	Method Open:Bool( path:String,mode:String )
	Method Length:Int()
	Method Offset:Int()
	Method Seek:Int( offset:Int )
	
End

Public

Class FileStream Extends Stream

	Method New( path:String,mode:String )
		_file=New BBFileStream
		If Not _file.Open( path,mode ) Error "Failed to open stream"
		Super.SetBBStream _file
	End
	
	Method Length:Int() Property
		Return _file.Length()
	End
	
	Method Offset:Int() Property
		Return _file.Offset()
	End
	
	Method Seek:Int( offset:Int )
		Return _file.Seek( offset )
	End
	
	Function Open:FileStream( path:String,mode:String )
		Local peer:=New BBFileStream
		If peer.Open( path,mode ) Return New FileStream( peer )
		Return Null
	End
	
	'***** INTERNAL *****
	Method New( file:BBFileStream )
		_file=file
	End

	Private
	
	Field _file:BBFileStream
	
End
