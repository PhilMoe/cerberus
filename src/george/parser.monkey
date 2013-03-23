
Import toker

Class Decl

	Field kind:String					'arg, const, global, function etc
	Field ident:String					'ID - for scopes, includes path.
	Field args:String					'generic args
	Field type:String					'Void, Int, Int[], Image, Image[] etc
	Field value:String					'for consts or arg defaults
	Field exts:String					'extends
	Field impls:String					'implements

	Field path:String
	Field decls:Decl[]
	Field docs:=New StringMap<String>
	
	Method New( kind:String )
		Self.kind=kind
	End
End

Class Parser

	Method New( src:String )
		_toker=New Toker(src)
		Bump
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
				Local args:=ParseIdentSeq()
				Parse ">"
				decl.args="<"+args+">"
			Endif
			
			If CParse("extends")
				decl.type="Extends "+ParseType()
			Endif
			
			If CParse("implements")
				If decl.type decl.type+=" "
				decl.type+="Implements "+ParseType()
			Endif
			
		Case "import"
		
			decl=New Decl(Parse())
			decl.ident=ParseIdent()

			If CParse("private") decl.kind+=" p"
			
		Case "const"
		
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			decl.type=ParseType()

			If CParse("=") decl.value=Parse()
			
		Case "global","field"
					
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			decl.type=ParseType()
			
		Case "method","function"
		
			decl=New Decl(Parse())
			decl.ident=ParseIdent()
			decl.type=ParseType()
			decl.args=ParseArgs()
			
			If decl.ident="New"
				decl.kind="ctor"
			Else If CParse("property") 
				decl.kind="prop"
			Endif
		
		Default 
			Err
		End
		
		Return decl
	End
	
	Private
	
	Field _toker:Toker
	Field _toke:String
	Field _tokeType:Int
	
		
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
		If TokeType<>Toker.Identifier Err
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
		If CParse(")") Return "()"
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
		Until Not CParse(",")
		Parse ")"
		Return "( "+args.Join(", ")+" )"
	End
	
	Method ParseType:String()
		CParse ":"
		Local ty:=""
		If TokeType=Toker.Identifier
			Local id:=ParseIdent()
			Select id.ToLower()
			Case "void","bool","int","float","string","object"
				ty+=id
			Default 
				ty+="[["+id+"]]"
			End
			If CParse("<")
				Local args:=New StringStack
				Repeat
					args.Push ParseType()
				Until Not CParse(",")
				Parse ">"
				ty+="&lt;"+args.Join(", ")+"&gt;"
			Endif
			While CParse("[")
				Parse "]"
				ty+="[]"
			Wend
		Endif
#rem		
		If CParse("(")
			If ty ty+=" "
			If CParse(")")
				ty+="( )"
			Else
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
				Until Not CParse(",")
				Parse ")"
				ty+="( "+args.Join(", ")+" )"
			Endif
		Endif
#end		
'		If Not ty Err

		Return ty
	End
	
End
