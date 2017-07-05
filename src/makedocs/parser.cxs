
Import toker

Class Decl

	Field kind:String					'arg, const, global, function etc
	Field ident:String					'ID - for scopes, includes path.
	Field type:String					'Void, Int, Int[], Image, Image[] etc
	Field init:String					'for consts or arg defaults
	Field exts:String					'extended type, eg: [[List]]<T> 
	Field impls:String					'implemented types, eg: [[IOnComplete]], [[IOnEvent]]

	Method New( kind:String,ident:String="" )
		Self.kind=kind
		Self.ident=ident
	End
	
End

Class Parser

	Method New( src:String )
		_toker=New Toker( src )
		Bump
	End
	
	Method SetText:Void( src:String )
		_toker.SetText src
		_toke=""
		_tokeType=0
		Bump
	End
	
	Method GetText:String()
		Return _toker.Text[_toker.Cursor..]	
	End
	
	Method ParseDecl:Decl()
		Local decl:Decl
		Select Toke
		Case "module"
		
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			
		Case "class","interface"
		
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			
			If CParse("<")
				decl.type="<"+ParseIdentSeq()+">"
				Parse ">"
			Endif
			
			If CParse("extends")
				decl.exts=ParseType()
			Endif
			
			If CParse("implements")
				decl.impls=ParseTypeSeq()
			Endif
			
		Case "import"
		
			Local kind:=Parse()
			If TokeType<>Toker.Identifier Return Null

			decl=New Decl( kind )
			decl.ident=ParseIdent()
			
		Case "const"
		
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			decl.type=ParseType()

			If CParse("=") decl.init=Parse()
			
		Case "global","field"
					
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			decl.type=ParseType()
			
		Case "method","function"
		
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			decl.type=ParseType()
			decl.type+=ParseArgs()
			
			If decl.ident="New"
				Local i:=decl.type.Find( "(" )
				If i<>-1 decl.type=decl.type[i..]
				decl.kind="ctor"
			Else If CParse("property") 
				decl.kind="property"
			Endif
		
		Default 
			Err
		End
		
		Return decl
	End
	
	Method GetToker:Toker()
		Return _toker
	End
	
	Method Err:Void( msg:String="Parse error" )
	
		Print "Toke="+Toke
		Local text:=_toker.Text
		Local cursor:=_toker.Cursor
		Print text[..cursor]+"<<<<<HERE?"+text[cursor..]
		Error msg
	End
	
	Method Toke:String() Property
		Return _toke
	End
	
	Method TokeType:Int() Property
		Return _tokeType
	End
	
	Method Bump:String()
		Repeat
			Local toke:=_toker.Bump()
			Local type:=_toker.TokeType
			If type=Toker.Eol Continue
			If type=Toker.Whitespace Continue
			_toke=toke.ToLower()
			_tokeType=type
			Return _toke
		Forever
	End
	
	Method SaveToker:Toker()
		Return New Toker( _toker )
	End
	
	Method RestoreToker:Void( toker:Toker )
		_toker=toker
		_toke=_toker.Toke().ToLower()
		_tokeType=_toker.TokeType()
	End
	
	Method Parse:String()
		Local tmp:=Toke
		Bump
		Return tmp
	End
	
	Method Parse:Void( toke:String )
		If Toke<>toke Err
		Bump
	End
	
	Method Parse:String( type:Int )
		If TokeType<>type Err
		Return Parse()
	End
	
	Method CParse:Bool( toke:String )
		If Toke<>toke Return False
		Bump
		Return True
	End

	Method CParse:String( type:Int )
		If TokeType<>type Return ""
		Return Parse()
	End
	
	Method ParseText:String()
		Local text:=_toker.Toke
		Bump
		Return text
	End

	Method ParseIdent:String()
		CParse "@"
		If TokeType<>Toker.Identifier Err
		Local id:=_toker.Toke
		Bump
		Return id
	End
	
	Method CParseIdent:String()
		If TokeType<>Toker.Identifier Return ""
		Local id:=_toker.Toke
		Bump
		Return id
	End
	
	Method ParseIdentSeq:String()
		Local args:=New StringStack
		Repeat
			args.Push ParseIdent()
		Until Not CParse(",")
		Return args.Join(",")
	End
	
	Method ParseTypeSeq:String()
		Local args:=New StringStack
		Repeat
			args.Push ParseType()
		Until Not CParse(",")
		Return args.Join(",")
	End
	
	Method ParseArgs:String()
		If Not CParse("(") Err
		If CParse(")") Return " ()"
		Local args:=New StringStack
		Repeat
			Local id:=ParseIdent()
			Local ty:=ParseType()
			If CParse("=")
				ty+="="
				While Toke<>"," And Toke<>")"
					ty+=Toke
					Bump
				Wend
			Endif
			args.Push id+":"+ty
		Until Not CParse( "," )
		Parse ")"
		Return " ( "+args.Join(", ")+" )"
	End
	
	Method ParseType:String()
		Local ty:=""
		If CParse( ":" ) Or TokeType=Toker.Identifier
			If CParse( "=" )
				If CParse( "new" )
					If TokeType=Toker.Identifier
						ty=ParseType()
					Endif
				Else
					Select TokeType
					Case Toker.IntLiteral
						Parse
						ty="int"
					Case Toker.FloatLiteral
						Parse
						ty="float"
					Case Toker.StringLiteral
						Parse
						ty="string"
					End
				Endif
				If Not ty
					Print "Inferred types not allowed: "+_toker.Text()
					Return ""
				Endif
			Else If TokeType=Toker.Identifier
				Local id:=ParseIdent()
				Select id.ToLower()
				Case "void","bool","int","float","string","object"
					ty+=id
				Default 
					ty+="[["+id+"]]"
				End
			Else
				Err
			Endif
		Else
			If CParse( "$" )
				ty="string"
			Else If CParse( "#" ) 
				ty="float"
			Else If CParse( "%" )
				ty="int"
			Else If CParse( "?" ) 
				ty="bool"
			Else
				ty="int"
			Endif
		Endif
		If CParse("<")
			Local args:=New StringStack
			Repeat
				args.Push ParseType()
			Until Not CParse(",")
			Parse ">"
			ty+="<"+args.Join(", ")+">"
		Endif
		While CParse( "[" )
			While TokeType And Toke<>"]"
				Parse
			Wend
			CParse "]"
			ty+="[]"
		Wend
		Return ty
	End
	
	Private
	
	Field _toker:Toker
	Field _toke:String
	Field _tokeType:Int
		
End
