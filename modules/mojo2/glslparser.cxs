
Const TOKE_EOF:=0
Const TOKE_IDENT:=1
Const TOKE_INTLIT:=2
Const TOKE_FLOATLIT:=3
Const TOKE_STRINGLIT:=4
Const TOKE_SYMBOL:=5
	
Const CHAR_QUOTE:=34
Const CHAR_PLUS:=43
Const CHAR_MINUS:=45
Const CHAR_PERIOD:=46
Const CHAR_UNDERSCORE:=95
Const CHAR_APOSTROPHE:=39

Function IsDigit:Bool( ch:Int )
	Return (ch>=48 And ch<58)
End

Function IsAlpha:Bool( ch:Int )
	Return (ch>=65 And ch<65+26) Or (ch>=97 And ch<97+26)
End

Function IsIdent:Bool( ch:Int )
	Return (ch>=65 And ch<65+26) Or (ch>=97 And ch<97+26) Or (ch>=48 And ch<58) Or ch=CHAR_UNDERSCORE
End

Class Parser

	Method New( text:String )
		SetText text
	End

	Method SetText:Void( text:String )
		_text=text
		_pos=0
		_len=_text.Length
		Bump
	End
	
	Method Bump:String()

		While _pos<_len
			Local ch:=_text[_pos]
			If ch<=32
				_pos+=1
				Continue
			Endif
			If ch<>CHAR_APOSTROPHE Exit
			_pos+=1
			While _pos<_len And _text[_pos]<>10
				_pos+=1
			Wend
		Wend
		
		If _pos=_len
			_toke=""
			_tokeType=TOKE_EOF
			Return _toke
		Endif
		
		Local pos:=_pos
		Local ch:=_text[_pos]
		_pos+=1
		
		If IsAlpha( ch ) Or ch=CHAR_UNDERSCORE
		
			While _pos<_len
				Local ch:=_text[_pos]
				If Not IsIdent( ch ) Exit
				_pos+=1
			Wend
			_tokeType=TOKE_IDENT
			
		Else If IsDigit( ch ) 
		
			While _pos<_len
				If Not IsDigit( _text[_pos] ) Exit
				_pos+=1
			Wend
			_tokeType=TOKE_INTLIT
			
		Else If ch=CHAR_QUOTE
		
			While _pos<_len
				Local ch:=_text[_pos]
				If ch=CHAR_QUOTE Exit
				_pos+=1
			Wend
			If _pos=_len Error "String literal missing closing quote"
			_tokeType=TOKE_STRINGLIT
			_pos+=1
			
		Else
			Local digraphs:=[":="]
			If _pos<_len
				Local ch:=_text[_pos]
				For Local t:=Eachin digraphs
					If ch=t[1]
						_pos+=1
						Exit
					Endif
				Next
			Endif
			_tokeType=TOKE_SYMBOL
		Endif
		
		_toke=_text[pos.._pos]
		
		Return _toke
	End
	
	Method Toke:String() Property
		Return _toke
	End
	
	Method TokeType:Int() Property
		Return _tokeType
	End
	
	Method CParse:Bool( toke:String )
		If _toke<>toke Return False
		Bump
		Return True
	End
	
	Method CParseIdent:String()
		If _tokeType<>TOKE_IDENT Return ""
		Local id:=_toke
		Bump
		Return id
	End
	
	Method CParseLiteral:String()
		If _tokeType<>TOKE_INTLIT And _tokeType<>TOKE_FLOATLIT And _tokeType<>TOKE_STRINGLIT Return ""
		Local id:=_toke
		Bump
		Return id
	End
	
	Method Parse:String()
		Local toke:=_toke
		Bump
		Return toke
	End
	
	Method Parse:Void( toke:String )
		If Not CParse( toke ) Error "Expecting '"+toke+"'"
	End
	
	Method ParseIdent:String()
		Local id:=CParseIdent()
		If Not id Error "Expecting identifier"
		Return id
	End
	
	Method ParseLiteral:String()
		Local id:=CParseLiteral()
		If Not id Error "Expecting literal"
		Return id
	End
	
	Private

	Field _text:String
	Field _pos:Int
	Field _len:Int
	Field _toke:String
	Field _tokeType:Int
	
End

Class GlslParser Extends Parser

	Method New( text:String )
		Super.New( text )
	End

	Method ParseType:String()
		Local id:=ParseIdent()
		Return id
	End
	
End
