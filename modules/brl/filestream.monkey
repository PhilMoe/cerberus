
Import brl.stream

#If LANG="cpp" Or LANG="java" Or LANG="cs"
#BRL_FILESTREAM_IMPLEMENTED=True
Import "native/filestream.${LANG}"
#Endif

#BRL_FILESTREAM_IMPLEMENTED=False
#If BRL_FILESTREAM_IMPLEMENTED="0"
#Error "Native FileStream class not found."
#Endif

Extern

Class BBFileStream Extends BBStream

	'Stream methods...
	Method Open:Bool( path:String,mode:String )
	Method Length:Int()
	Method Position:Int()
	Method Seek:Int( position:Int )
	
End

Public

Class FileStream Extends Stream

	Method New( path:String,mode:String )
		_stream=New BBFileStream
		If Not _stream.Open( path,mode ) Error "Failed to open stream"
	End
	
	Method Close:Void()
		If _stream 
			_stream.Close
			_stream=Null
		Endif
	End
	
	Method Eof:Int()
		Return _stream.Eof()
	End
	
	Method Length:Int()
		Return _stream.Length()
	End
	
	Method Position:Int()
		Return _stream.Position()
	End
	
	Method Seek:Int( position:Int )
		Return _stream.Seek( position )
	End
	
	Method Read:Int( buffer:DataBuffer,offset:Int,count:Int )
		Return _stream.Read( buffer,offset,count )
	End
	
	Method Write:Int( buffer:DataBuffer,offset:Int,count:Int )
		Return _stream.Write( buffer,offset,count )
	End
	
	Function Open:FileStream( path:String,mode:String )
		Local stream:=New BBFileStream
		If stream.Open( path,mode ) Return New FileStream( stream )
		Return Null
	End
	
	'***** INTERNAL *****
	Method New( stream:BBFileStream )
		_stream=stream
	End

	Private
	
	Field _stream:BBFileStream
	
End
