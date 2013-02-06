
Import "native/databuffer.${LANG}"

Extern

Class BBDataBuffer

	Method Discard:Void()

	Method Length:Int() Property
	
	Method PokeByte:Void( addr:Int,value:Int )
	Method PokeShort:Void( addr:Int,value:Int )
	Method PokeInt:Void( addr:Int,value:Int )
	Method PokeFloat:Void( addr:Int,value:Float )
	
	Method PeekByte:Int( addr:Int )
	Method PeekShort:Int( addr:Int )
	Method PeekInt:Int( addr:Int )
	Method PeekFloat:Float( addr:Int )
	
Private
	
	Method _New:Bool( length:Int )
	Method _Load:Bool( path:String )

End

Public

'OK, for now no error checking of count/address/offset params. These should probably be:
'
'Assert( count>=0 )
'Assert( offset>=0 And offset<=array.Length )      'Same for buffers/arrays. Note: OK to index 'one past last'.
'
'count is always clipped however.

Class DataBuffer Extends BBDataBuffer

	Method New( length:Int )
		If Not _New( length ) Error "Allocate DataBuffer failed"
	End
	
	'Native-ize me!
	Method CopyBytes:Void( address:Int,dst:DataBuffer,dstaddress:Int,count:Int )
	
		If address+count>Length count=Length-address
		If dstaddress+count>dst.Length count=dst.Length-dstaddress
		
		If dstaddress<=address
			For Local i:=0 Until count
				dst.PokeByte dstaddress+i,PeekByte( address+i )
			Next
		Else
			For Local i:=count-1 To 0 Step -1
				dst.PokeByte dstaddress+i,PeekByte( address+i )
			Next
		Endif
	End
	
	Method PeekBytes:Int[]( address:Int=0 )
	
		Return PeekBytes( address,Length )

	End
	
	Method PeekBytes:Int[]( address:Int,count:Int )
	
		If address+count>Length count=Length-address
		
		Local bytes:=New Int[count]
		PeekBytes address,bytes,0,count
		Return bytes
	End
	
	Method PeekBytes:Void( address:Int,bytes:Int[],offset:Int=0 )
	
		PeekBytes address,bytes,offset,Length

	End
	
	Method PeekBytes:Void( address:Int,bytes:Int[],offset:Int,count:Int )

		If address+count>Length count=Length-address
		If offset+count>bytes.Length count=bytes.Length-offset
		
		For Local i:=0 Until count
			bytes[offset+i]=PeekByte( address+i )
		Next
	End
	
	Method PokeBytes:Void( address:Int,bytes:Int[],offset:Int=0 )

		PokeBytes address,bytes,offset,Length
	End

	Method PokeBytes:Void( address:Int,bytes:Int[],offset:Int,count:Int )

		If address+count>Length count=Length-address
		If offset+count>bytes.Length count=bytes.Length-offset

		For Local i:=0 Until count
			PokeByte address+i,bytes[offset+i]
		Next
	End
	
	Method PeekString:String( address:Int )
		Return PeekString( Length-address )
	End
	
	Method PeekString:String( address:Int,count:Int )
		Return String.FromChars( PeekBytes( address,count ) )
	End

	Method PokeString:Void( address:Int,str:String )
		Local chars:=New Int[str.Length]
		For Local i:=0 Until str.Length
			chars[i]=str[i]
		Next
		PokeBytes address,chars
'		PokeBytes address,str.ToChars()
	End
	
	Function Load:DataBuffer( path:String )
		Local buf:=New DataBuffer
		If buf._Load( path ) Return buf
		Return Null
	End
	
	'***** INTERNAL *****
	Method GetBBDataBuffer:BBDataBuffer()
		Return Self
	End

End
