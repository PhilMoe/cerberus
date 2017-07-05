
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

	Method Close:Void() Abstract
	
	Method Eof:Int() Property Abstract
	
	Method Length:Int() Property Abstract
	
	Method Position:Int() Property Abstract
	
	Method Seek:Int( position:Int ) Abstract
	
	Method Read:Int( buffer:DataBuffer,offset:Int,count:Int ) Abstract
	
	Method Write:Int( buffer:DataBuffer,offset:Int,count:Int ) Abstract
	
	Method ReadAll:Void( buffer:DataBuffer,offset:Int,count:Int )
		While count>0
			Local n:=Read( buffer,offset,count )
			If n<=0 ReadError
			offset+=n
			count-=n
		Wend
	End
	
	Method ReadAll:DataBuffer()
		Local bufs:=New Stack<DataBuffer>
		Local buf:=New DataBuffer( 4096 ),off:=0,len:=0
		Repeat
			Local n:=Read( buf,off,4096-off )
			If n<=0 Exit
			off+=n
			len+=n
			If off=4096
				off=0
				bufs.Push buf
				buf=New DataBuffer( 4096 )
			Endif
		Forever
		Local data:=New DataBuffer( len )
		off=0
		For Local tbuf:=Eachin bufs
			tbuf.CopyBytes 0,data,off,4096
			tbuf.Discard()
			off+=4096
		Next
		buf.CopyBytes 0,data,off,len-off
		buf.Discard()
		Return data
	End
	
	Method WriteAll:Void( buffer:DataBuffer,offset:Int,count:Int )
		While count>0
			Local n:=Write( buffer,offset,count )
			If n<=0 WriteError
			offset+=n
			count-=n
		Wend
	End
	
	Method Skip:Void( count:Int )
		While count>0
			Local n:=Read( _tmp,0,Min( count,BUF_SZ ) )
			If n<=0 ReadError
			count-=n
		Wend
	End
	
	Method ReadByte:Int()
		ReadAll _tmp,0,1
		Return _tmp.PeekByte( 0 )
	End
	
	Method ReadShort:Int()
		ReadAll _tmp,0,2
		Return _tmp.PeekShort( 0 )
	End
	
	Method ReadInt:Int()
		ReadAll _tmp,0,4
		Return _tmp.PeekInt( 0 )
	End
	
	Method ReadFloat:Float()
		ReadAll _tmp,0,4
		Return _tmp.PeekFloat( 0 )
	End
	
	Method ReadString:String( count:Int,encoding:String="utf8" )
		Local buf:=New DataBuffer( count )
		ReadAll( buf,0,count )
		Return buf.PeekString( 0,encoding )
	End
	
	Method ReadString:String( encoding:String="utf8" )
		Local buf:=ReadAll()
		Return buf.PeekString( 0,encoding )
	End
	
	Method ReadLine:String()
		Local buf:=New Stack<Int>
		While Not Eof()
			Local n:=Read( _tmp,0,1 )
			If Not n Exit
			Local ch:=_tmp.PeekByte( 0 )
			If Not ch Or ch=10 Exit
			If ch<>13 buf.Push ch
		Wend
		Return String.FromChars(buf.ToArray())
	End
	
	Method WriteByte:Void( value:Int )
		_tmp.PokeByte 0,value
		WriteAll _tmp,0,1
	End
	
	Method WriteShort:Void( value:Int )
		_tmp.PokeShort 0,value
		WriteAll _tmp,0,2
	End
	
	Method WriteInt:Void( value:Int )
		_tmp.PokeInt 0,value
		WriteAll _tmp,0,4
	End
	
	Method WriteFloat:Void( value:Float )
		_tmp.PokeFloat 0,value
		WriteAll _tmp,0,4
	End
	
	Method WriteString:Void( value:String,encoding:String="utf8" )
		Local buf:=New DataBuffer( value.Length*3 )
		Local len:=buf.PokeString( 0,value )
		WriteAll buf,0,len
	End
	
	Method WriteLine:Void( str:String )
		For Local ch:=Eachin str
			WriteByte ch
		Next
		WriteByte 13
		WriteByte 10
	End
	
	'***** INTERNAL *****
	Method GetNativeStream:BBStream()
		Return Null
	End
	
	Private
	
	Const BUF_SZ=4096

	Global _tmp:=New DataBuffer( BUF_SZ )
	
	Method ReadError:Void()
		Throw New StreamReadError( Self )
	End
	
	Method WriteError:Void()
		Throw New StreamWriteError( Self )
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
