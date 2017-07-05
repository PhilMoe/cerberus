
Private

Function ThrowError:Void()
'	DebugStop
	Throw New JsonError
End

Public

Class JsonError Extends Throwable
End

Class JsonValue Abstract

	Method BoolValue:Bool()
		ThrowError
	End
	
	Method IntValue:Int()
		ThrowError
	End
	
	Method FloatValue:Float()
		ThrowError
	End
	
	Method StringValue:String()
		ThrowError
	End
	
	Method ToJson:String()
		Local buf:=New StringStack
		PushJson( buf )
		Return buf.Join()
	End
		
	Method PushJson:Void( buf:StringStack )
		buf.Push ToJson()
	End

End

Class JsonObject Extends JsonValue

	Method New()
		_data=New StringMap<JsonValue>
	End
	
	Method New( json:String )
		_data=(New JsonParser( json ) ).ParseObject()
	End

	Method New( data:StringMap<JsonValue> )
		_data=data
	End
	
	Method Contains:Bool( key:String )
		Return _data.Contains( key )
	End
	
	Method Set:Void( key:String,value:JsonValue )
		_data.Set( key,value )
	End

	Method SetBool:Void( key:String,value:Bool )
		Set( key,New JsonBool( value ) )
	End
	
	Method SetInt:Void( key:String,value:Int )
		Set( key,New JsonNumber( value ) )
	End
	
	Method SetFloat:Void( key:String,value:Float )
		Set( key,New JsonNumber( value ) )
	End
	
	Method SetString:Void( key:String,value:String )
		Set( key,New JsonString( value ) )
	End
	
	Method Get:JsonValue( key:String,defval:JsonValue=Null )
		If Not _data.Contains( key ) Return defval
		Local val:=_data.Get( key )
		If val Return val
		Return JsonNull.Instance()
	End
	
	Method GetBool:Bool( key:String,defval:Bool=False )
		If Not _data.Contains( key ) Return defval
		Return Get( key ).BoolValue()
	End
		
	Method GetInt:Int( key:String,defval:Int=0 )
		If Not _data.Contains( key ) Return defval
		Return Get( key ).IntValue()
	End
		
	Method GetFloat:Float( key:String,defval:Float=0 )
		If Not _data.Contains( key ) Return defval
		Return Get( key ).FloatValue()
	End
		
	Method GetString:String( key:String,defval:String="" )
		If Not _data.Contains( key ) Return defval
		Return Get( key ).StringValue()
	End
	
	Method GetData:StringMap<JsonValue>()
		Return _data
	End
		
	Method PushJson:Void( buf:StringStack )
		buf.Push "{"
		Local t:=False
		For Local it:=Eachin _data
			If t buf.Push ","
			buf.Push "~q"+it.Key.Replace( "~q","\~q" )+"~q:"
			If it.Value<>Null it.Value.PushJson( buf ) Else buf.Push "null"
			t=True
		Next
		buf.Push "}"
	End
	
	Private
	
	Field _data:StringMap<JsonValue>
	
End

Class JsonArray Extends JsonValue

	Method New( length:Int )
		_data=New JsonValue[length]
	End
	
	Method New( data:JsonValue[] )
		_data=data
	End
	
	Method Length:Int() Property
		Return _data.Length
	End
	
	Method Set:Void( index:Int,value:JsonValue )
		If index<0 Or index>=_data.Length ThrowError
		_data[index]=value
	End
	
	Method SetBool:Void( index:Int,value:Bool )
		Set( index,New JsonBool( value ) )
	End
	
	Method SetInt:Void( index:Int,value:Int )
		Set( index,New JsonNumber( value ) )
	End
	
	Method SetFloat:Void( index:Int,value:Float )
		Set( index,New JsonNumber( value ) )
	End
	
	Method SetString:Void( index:Int,value:String )
		Set( index,New JsonString( value ) )
	End
	
	Method Get:JsonValue( index:Int )
		If index<0 Or index>=_data.Length ThrowError
		Local val:=_data[index]
		If val Return val
		Return JsonNull.Instance()
	End
	
	Method GetBool:Bool( index:Int )
		Return Get( index ).BoolValue()
	End
	
	Method GetInt:Int( index:Int )
		Return Get( index ).IntValue()
	End
	
	Method GetFloat:Float( index:Int )
		Return Get( index ).FloatValue()
	End
	
	Method GetString:String( index:Int )
		Return Get( index ).StringValue()
	End

	Method GetData:JsonValue[]()
		Return _data
	End
	
	Method PushJson:Void( buf:StringStack )
		buf.Push "["
		Local t:=False
		For Local value:=Eachin _data
			If t buf.Push ","
			If value<>Null value.PushJson( buf ) Else buf.Push "null"
			t=True
		Next
		buf.Push "]"
	End

	Private
	
	Field _data:JsonValue[]
	
End

Class JsonNull Extends JsonValue

	Method ToJson:String()
		Return "null"
	End
	
	Function Instance:JsonNull()
		Return _instance
	End
	
	Private
	
	Global _instance:=New JsonNull
	
End

Class JsonBool Extends JsonValue

	Method New( value:Bool )
		_value=value
	End
	
	Method BoolValue:Bool()
		Return _value
	End
	
	Method ToJson:String()
		If _value Return "true"
		Return "false"
	End
	
	Function Instance:JsonBool( value:Bool )
		If value Return _true
		Return _false
	End
	
	Private
	
	Field _value:Bool
	
	Global _true:=New JsonBool( True )
	Global _false:=New JsonBool( False )
	
End

Class JsonString Extends JsonValue

	Method New( value:String )
		_value=value
	End
	
	Method StringValue:String()
		Return _value
	End
	
	Method ToJson:String()
		Return "~q"+_value.Replace( "~q","\~q" )+"~q"
	End
	
	Function Instance:JsonString( value:String )
		If value Return New JsonString( value )
		Return _null
	End
	
	Private
	
	Field _value:String
	
	Global _null:=New JsonString( "" )
	
End

Class JsonNumber Extends JsonValue

	Method New( value:String )
		'error check value!
		_value=value
	End
	
	Method IntValue:Int()
		Return Int( _value )
	End
	
	Method FloatValue:Float()
		Return Float( _value )
	End	
	
	Method ToJson:String()
		Return _value
	End
	
	Function Instance:JsonNumber( value:String )
		If value<>"0" Return New JsonNumber( value )
		Return _zero
	End
	
	Private
	
	Field _value:String
	
	Global _zero:=New JsonNumber( "0" )
End

Class JsonParser

	Method New( json:String )
		_text=json
		Bump
	End
	
	Method ParseValue:JsonValue()
		If TokeType=T_STRING Return JsonString.Instance( ParseString() )
		If TokeType=T_NUMBER Return JsonNumber.Instance( ParseNumber() )
		If Toke="{" Return New JsonObject( ParseObject() )
		If Toke="[" Return New JsonArray( ParseArray() )
		If CParse("true") Return JsonBool.Instance( True )
		If CParse("false") Return JsonBool.Instance( False )
		If CParse("null") Return JsonNull.Instance()
		ThrowError
	End

	Private
	
	Const T_EOF:=0
	Const T_STRING:=1
	Const T_NUMBER:=2
	Const T_SYMBOL:=3
	Const T_IDENT:=4
	
	Field _text:String
	Field _toke:String
	Field _type:Int
	Field _pos:Int
	
	Method GetChar:Int()
		If _pos=_text.Length ThrowError
		_pos+=1
		Return _text[_pos-1]
	End
	
	Method PeekChar:Int()
		If _pos=_text.Length Return 0
		Return _text[_pos]
	End
	
	Method ParseChar:Void( chr:Int )
		If _pos>=_text.Length Or _text[_pos]<>chr ThrowError
		_pos+=1
	End
	
	Method CParseChar:Bool( chr:Int )
		If _pos>=_text.Length Or _text[_pos]<>chr Return False
		_pos+=1
		Return True
	End
	
	Method CParseDigits:Bool()
		Local p:=_pos
		While _pos<_text.Length And _text[_pos]>=48 And _text[_pos]<=57
			_pos+=1
		Wend
		Return _pos>p
	End
	
	Method Bump:String()
	
		While _pos<_text.Length And _text[_pos]<=32
			_pos+=1
		Wend
		
		If _pos=_text.Length
			_toke=""
			_type=T_EOF
			Return _toke
		Endif
		
		Local pos:=_pos
		Local chr:=GetChar()
		
		If chr=34
			Repeat
				Local chr:=GetChar()
				If chr=34 Exit
				If chr=92 GetChar()
			Forever
			_type=T_STRING
		Else If chr=45 Or (chr>=48 And chr<=57)
			If chr=45 '-
				chr=GetChar()
				If chr<48 Or chr>57 ThrowError
			Endif
			If chr<>48 '0
				CParseDigits()
			End
			If CParseChar( 46 )	'.
				CParseDigits()
			Endif
			If CParseChar( 69 ) Or CParseChar( 101 ) 'e E
				If PeekChar()=43 Or PeekChar()=45 GetChar()	'+ -
				If Not CParseDigits() ThrowError
			Endif
			_type=T_NUMBER
		Else If (chr>=65 And chr<91) Or (chr>=97 And chr<123)
			chr=PeekChar()
			While (chr>=65 And chr<91) Or (chr>=97 And chr<123)
				GetChar()
				chr=PeekChar()
			Wend
			_type=T_IDENT
		Else
			_type=T_SYMBOL
		Endif
		_toke=_text[pos.._pos]
		Return _toke
	End
	
	Method Toke:String() Property
		Return _toke
	End
	
	Method TokeType:Int() Property
		Return _type
	End
	
	Method CParse:Bool( toke:String )
		If toke<>_toke Return False
		Bump
		Return True
	End
	
	Method Parse:Void( toke:String )
		If Not CParse( toke ) ThrowError
	End

	Method ParseObject:StringMap<JsonValue>()
		Parse( "{" )
		Local map:=New StringMap<JsonValue>
		If CParse( "}" ) Return map
		Repeat
			Local name:=ParseString()
			Parse( ":" )
			Local value:=ParseValue()
			map.Set name,value
		Until Not CParse( "," )
		Parse( "}" )
		Return map
	End
	
	Method ParseArray:JsonValue[]()
		Parse( "[" )
		If CParse( "]" ) Return []
		Local stack:=New Stack<JsonValue>
		Repeat
			Local value:=ParseValue()
			stack.Push value
		Until Not CParse( "," )
		Parse( "]" )
		Return stack.ToArray()
	End
	
	Method ParseString:String()
		If TokeType<>T_STRING ThrowError
		Local toke:=Toke[1..-1]
		Local i:=toke.Find( "\" )
		If i<>-1
			Local frags:=New StringStack,p:=0,esc:=""
			Repeat
				If i+1>=toke.Length ThrowError
				frags.Push toke[p..i]
				Select toke[i+1]
				Case 34  esc="~q"					'\"
				Case 92  esc="\"					'\\
				Case 47  esc="/"					'\/
				Case 98  esc=String.FromChar( 8 )	'\b
				Case 102 esc=String.FromChar( 12 )	'\f
				Case 114 esc=String.FromChar( 13 )	'\r
				Case 110 esc=String.FromChar( 10 )	'\n
				Case 117							'\uxxxx
					If i+6>toke.Length ThrowError
					Local val:=0
					For Local j:=2 Until 6
						Local chr:=toke[i+j]
						If chr>=48 And chr<58
							val=val Shl 4 | (chr-48)
						Else If chr>=65 And chr<123
							chr&=31
							If chr<1 Or chr>6 ThrowError
							val=val Shl 4 | (chr+9)
						Else
							ThrowError
						Endif
					Next
					esc=String.FromChar( val )
					i+=4
				Default 
					ThrowError
				End
				frags.Push esc
				p=i+2
				i=toke.Find( "\",p )
				If i<>-1 Continue
				frags.Push toke[p..]
				Exit
			Forever
			toke=frags.Join()
		Endif
		Bump
		Return toke
	End
	
	Method ParseNumber:String()
		If TokeType<>T_NUMBER ThrowError
		Local toke:=Toke
		Bump
		Return toke
	End

End
