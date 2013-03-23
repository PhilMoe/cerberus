
Import brl.databuffer

#If LANG="cpp" Or LANG="java" Or LANG="cs" Or LANG="js" Or LANG="as"
#BRL_STREAM_IMPLEMENTED=True
Import "native/stream.${LANG}"
#Endif

#BRL_STREAM_IMPLEMENTED=False
#If BRL_STREAM_IMPLEMENTED

Extern

Class BBStream

	'blah...
	Method Eof:Int()
	Method Close:Void()
	Method Length:Int()
	Method Position:Int()
	Method Seek:Int( position:Int )
	Method Read:Int( buf:DataBuffer,offset:Int,count:Int )
	Method Write:Int( buf:DataBuffer,offset:Int,count:Int )
	
End

Public

#Endif

Class Stream

	Method Eof:Int() Abstract
	
	Method Close:Void() Abstract
	
	Method Length:Int() Abstract
	
	Method Position:Int() Abstract
	
	Method Seek:Int( position:Int ) Abstract
	
	Method Read:Int( buffer:DataBuffer,offset:Int,count:Int ) Abstract
	
	Method Write:Int( buffer:DataBuffer,offset:Int,count:Int ) Abstract
	
	Method Skip:Int( count:Int )
		Local n:=0
		While n<count
			Local t:=Read( _tmpbuf,0,Min( count-n,BUF_SZ ) )
			If Not t And Eof() Throw New StreamReadError( Self )
			n+=t
		Wend
		Return n
	End
	
	Method ReadByte:Int()
		_Read 1
		Return _tmpbuf.PeekByte( 0 )
	End
	
	Method ReadShort:Int()
		_Read 2
		Return _tmpbuf.PeekShort( 0 )
	End
	
	Method ReadInt:Int()
		_Read 4
		Return _tmpbuf.PeekInt( 0 )
	End
	
	Method ReadFloat:Float()
		_Read 4
		Return _tmpbuf.PeekFloat( 0 )
	End
	
	Method ReadLine:String()
		Local buf:=New Stack<Int>
		While Not Eof()
			Local n:=Read( _tmpbuf,0,1 )
			If Not n Exit
			Local ch:=_tmpbuf.PeekByte( 0 )
			If Not ch Or ch=10 Exit
			If ch<>13 buf.Push ch
		Wend
		Return String.FromChars(buf.ToArray())
	End
	
	Method WriteByte:Void( value:Int )
		_tmpbuf.PokeByte 0,value
		_Write 1
	End
	
	Method WriteShort:Void( value:Int )
		_tmpbuf.PokeShort 0,value
		_Write 2
	End
	
	Method WriteInt:Void( value:Int )
		_tmpbuf.PokeInt 0,value
		_Write 4
	End
	
	Method WriteFloat:Void( value:Float )
		_tmpbuf.PokeFloat 0,value
		_Write 4
	End
	
	Method WriteLine:Void( str:String )
		For Local ch:=Eachin str
			WriteByte ch
		Next
		WriteByte 13
		WriteByte 10
	End

	Private
	
	Const BUF_SZ=4096

	Global _tmpbuf:=New DataBuffer( BUF_SZ )
	
	Method _Read:Void( n:Int )
		Local i:=0
		Repeat
			i+=Read( _tmpbuf,i,n-i )
			If i=n Return
			If Eof() Throw New StreamReadError( Self )
		Forever
	End
	
	Method _Write:Void( n:Int )
		If Write( _tmpbuf,0,n )<>n Throw New StreamWriteError( Self )
	End
	
End

Class StreamError Extends Throwable

	Method New( stream:Stream )
		_stream=stream
	End
	
	Method GetStream:Stream()
		Return _stream
	End
		
	Method ToString:String() Abstract
	
	Private
	
	Field _stream:Stream
End

Class StreamReadError Extends StreamError

	Method New( stream:Stream )
		Super.New stream
	End

	Method ToString:String()
		Return "Error reading from stream"
	End
		
End

Class StreamWriteError Extends StreamError

	Method New( stream:Stream )
		Super.New stream
	End

	Method ToString:String()
		Return "Error writing to stream"
	End
	
End
