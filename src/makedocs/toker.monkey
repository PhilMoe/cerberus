
Import stringutil

Class Toker

	Const Eof:=0
	Const Eol:=1
	Const Whitespace:=2
	Const Identifier:=3
	Const IntLiteral:=4
	Const FloatLiteral:=5
	Const StringLiteral:=6
	Const Symbol:=7

	Method New( text:String )
		_text=text
		_len=_text.Length
	End
	
	Method New( toker:Toker )
		_text=toker._text
		_len=toker._len
		_toke=toker._toke
		_type=toker._type
		_pos=toker._pos
	End
	
	Method SetText:Void( text:String )
		_text=text
		_len=_text.Length
		_toke=""
		_type=0
		_pos=0
	End
	
	Method Text:String() Property
		Return _text
	End
	
	Method Cursor:Int() Property
		return _pos
	End
	
	Method Bump:String()
	
		If _pos=_len
			_toke=""
			_type=Eof
			Return _toke
		Endif

		Local stp:=_pos
		Local chr:=Chr()
		Local str:=Str()
		_pos+=1
		
		If chr=10
			_type=Eol
		Else If str="'"
			While Chr() And Chr()<>10
				_pos+=1
			Wend
			If Chr()=10 _pos+=1
			_type=Eol
		Else If chr<=32
			While Chr() And Chr()<=32
				_pos+=1
			Wend
			_type=Whitespace
		Else If IsAlpha(chr) Or str="_"
			While IsAlpha(Chr()) Or IsDigit(Chr()) Or Str()="_"
				_pos+=1
			Wend
			While Chr()=46	' .
				_pos+=1
				While IsAlpha(Chr()) Or IsDigit(Chr()) Or Str()="_"
					_pos+=1
				Wend
			Wend
			_type=Identifier
		Else If chr=34
			While Chr()<>34 And Chr()<>10
				_pos+=1
			Wend
			If Chr()=34 _pos+=1
			_type=StringLiteral
		Else If IsDigit(chr) Or str="." And IsDigit(Chr())
			_type=IntLiteral
			If str="." _type=FloatLiteral
			While IsDigit(Chr())
				_pos+=1
			Wend
			If _type=IntLiteral And Str()="." And IsDigit(Chr(1))
				_type=FloatLiteral
				_pos+=2
				While IsDigit(Chr())
					_pos+=1
				Wend
			Endif
			If Str().ToLower()="e"
				_type=FloatLiteral
				_pos+=1
				If Str()="+" Or Str()="-" _pos+=1
				While IsDigit(Chr())
					_pos+=1
				Wend
			Endif
		Else If str="%" And IsBinDigit(Chr())
			_type=IntLiteral
			_pos+=1
			While IsBinDigit(Chr())
				_pos+=1
			Wend
		Else If str="$" And IsHexDigit(Chr())
			_type=IntLiteral
			_pos+=1
			While IsHexDigit(Chr())
				_pos+=1
			Wend
		Else
			While Chr()=35	' #
				_pos+=1
			Wend
			_type=Symbol
		Endif
		
		_toke=_text[stp.._pos]
		Return _toke
	End
	
	Method Toke:String() Property
		Return _toke
	End
	
	Method TokeType:Int() Property
		Return _type
	End

Private

	Field _text:String
	Field _len:Int
	Field _toke:String
	Field _type:Int
	Field _pos:Int

	Method Chr:Int( offset:Int=0 )
		If _pos+offset<_len Return _text[_pos+offset]
		Return 0
	End
	
	Method Str:String( offset:Int=0 )
		If _pos+offset<_len Return _text[_pos+offset.._pos+offset+1]
		Return ""
	End
	
	Method IsSpace( ch )
		Return ch<=32
	'	Return ch<=Asc(" ")
	End
	
	Method IsDigit( ch )
		Return ch>=48 And ch<=57
	'	Return ch>=Asc("0") And ch<=Asc("9")
	End
	
	Method IsAlpha( ch )
		Return (ch>=65 And ch<=90) Or (ch>=97 And ch<=122)
	'	Return (ch>=Asc("A") And ch<=Asc("Z")) Or (ch>=Asc("a") And ch<=Asc("z"))
	End
	
	Method IsBinDigit( ch )
		Return ch=48 Or ch=49
	'	Return ch=Asc("0") Or ch=Asc("1")
	End
	
	Method IsHexDigit( ch )
		Return (ch>=48 And ch<=57) Or (ch>=65 And ch<=70) Or (ch>=97 And ch<=102)
	'	Return IsDigit(ch) Or (ch>=Asc("A") And ch<=Asc("F")) Or (ch>=Asc("a") And ch<=Asc("f"))
	End
		
	
End
