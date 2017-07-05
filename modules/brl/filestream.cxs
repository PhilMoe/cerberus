
Import brl.stream

#If Not BRL_FILESTREAM_IMPLEMENTED
#If LANG="cpp" Or LANG="java" Or LANG="cs"
#BRL_FILESTREAM_IMPLEMENTED=True
Import "native/filestream.${LANG}"
#Endif
#Endif

#If Not BRL_FILESTREAM_IMPLEMENTED
#Error "Native FileStream class not implemented."
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
		_stream=OpenStream( path,mode )
		If Not _stream Error "Failed to open stream"
	End
	
	Method Close:Void()
		If Not _stream Return
		_stream.Close
		_stream=Null
	End
	
	Method Eof:Int() Property
		Return _stream.Eof()
	End
	
	Method Length:Int() Property
		Return _stream.Length()
	End
	
	Method Position:Int() Property
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
		Local stream:=OpenStream( path,mode )
		If stream Return New FileStream( stream )
		Return Null
	End
	
	Private
	
	Method New( stream:BBFileStream )
		_stream=stream
	End
	
	Function OpenStream:BBFileStream( path:String,mode:String )
		Local stream:=New BBFileStream
		Local fmode:=mode
		If fmode="a" fmode="u"
		If Not stream.Open( path,fmode ) Return Null
		If mode="a" stream.Seek stream.Length()
		Return stream
	End

	Field _stream:BBFileStream
	
End
