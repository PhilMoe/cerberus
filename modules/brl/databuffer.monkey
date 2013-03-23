
#If LANG="cpp" Or LANG="java" Or LANG="cs" Or LANG="js" Or LANG="as"
#BRL_DATABUFFER_IMPLEMENTED=True
Import "native/databuffer.${LANG}"
#Endif

#BRL_DATABUFFER_IMPLEMENTED=False
#If BRL_DATABUFFER_IMPLEMENTED="0"
#Error "Native DataBuffer class not found."
#Endif

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
	
	Method Slice:DataBuffer( start:Int )
		Return Slice( start,Length )
	End
	
	Method Slice:DataBuffer( start:Int,term:Int )
		If start<0 start=0
		If term>Length term=Length
		Local len:=term-start
		If len<=0 Return New DataBuffer( 0 )
		Local buf:=New DataBuffer( len )
		CopyBytes start,buf,0,len
		Return buf
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
	
	Method PeekString:String( address:Int,encoding:String="utf8" )
		Return PeekString( address,Length-address,encoding )
	End
	
	Method PeekString:String( address:Int,count:Int,encoding:String="utf8" )

		Select encoding
		Case "utf8"
			Local p:=PeekBytes( address,count )
			Local i:=0,e:=p.Length,err:=False
			Local q:=New Int[e],j:=0
			While i<e
				Local c:=p[i] & $ff
				i+=1
				If c & $80
					If (c & $e0)=$c0
						If i>=e Or (p[i] & $c0)<>$80
							err=True
							Exit
						Endif
						c=(c & $1f) Shl 6 | (p[i] & $3f)
						i+=1
					Else If (c & $f0)=$e0
						If i+1>=e Or (p[i] & $c0)<>$80 Or (p[i+1] & $c0)<>$80
							err=True
							Exit
						Endif
						c=(c & $0f) Shl 12 | (p[i] & $3f) Shl 6 | (p[i+1] & $3f)
						i+=2
					Else
						err=True
						Exit
					Endif
				Endif
				q[j]=c
				j+=1
			Wend
			If err
				'UTF8 encoding error! 
				Return String.FromChars( p )
			Endif
			If j<e q=q[..j]
			Return String.FromChars( q )
		Case "ascii"
			Local p:=PeekBytes( address,count )
			For Local i:=0 Until p.Length
				p[i]&=$ff
			Next
			Return String.FromChars( p )
		End
		
		Error "Invalid string encoding:"+encoding
	End
	
	Method PokeString:Int( address:Int,str:String,encoding:String="utf8" )
	
		Select encoding
		Case "utf8"
			Local p:=str.ToChars()
			Local i:=0,e:=p.Length
			Local q:=New Int[e*3],j:=0
			While i<e
				Local c:=p[i] & $ffff
				i+=1
				If c<$80
					q[j]=  c
					j+=1
				Else If c<$800
					q[j]=  $c0 | (c Shr 6)
					q[j+1]=$80 | (c & $3f)
					j+=2
				Else
					q[j]=  $e0 | (c Shr 12)
					q[j+1]=$80 | (c Shr 6 & $3f)
					q[j+2]=$80 | (c & $3f)
					j+=3
				Endif
			Wend
			PokeBytes address,p,0,j
			Return j
		Case "ascii"
			PokeBytes address,str.ToChars(),0,str.Length
			Return str.Length
		End
		
		Error "Invalid string encoding:"+encoding
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
