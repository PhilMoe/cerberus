
Import parser

Function EvalExpr:Expr( toker:Toker )

	Local buf:=New StringStack
	While toker.Toke And toker.Toke<>"~n" And toker.TokeType<>TOKE_LINECOMMENT
		buf.Push toker.Toke
		toker.NextToke
	Wend
	Local source:=buf.Join( "" )
	
	toker=New Toker( "",source )
	Local parser:=New Parser( toker,Null )
	Local expr:=parser.ParseExpr().Semant()
	
	Return expr
End

Function EvalBool:Bool( toker:Toker )
	Local expr:=EvalExpr( toker )
	If Not BoolType( expr.exprType ) expr=expr.Cast( Type.boolType,CAST_EXPLICIT )
	If expr.Eval() Return True
	Return False
End

Function EvalText:String( toker:Toker )

	Local expr:=EvalExpr( toker )
	Local val:=expr.Eval()
	
	If StringType( expr.exprType ) 
		Return EvalConfigTags( val )
	Endif
	
	If BoolType( expr.exprType )
		If val Return "True"
		Return "False"
	End
	
	Return val
End

Function PreProcess$( path$,mdecl:ModuleDecl=Null )

	Local cnest,ifnest,line,source:=New StringStack
	
	PushEnv GetConfigScope()
	
	Local p_cd:=GetConfigVar( "CD" )
	Local p_modpath:=GetConfigVar( "MODPATH" )
	
	SetConfigVar "CD",ExtractDir( RealPath( path ) )
	If mdecl SetConfigVar "MODPATH",mdecl.rmodpath Else SetConfigVar "MODPATH",""
	
	Local toker:=New Toker( path,LoadString( path ) )
	toker.NextToke
	
	Local attrs:=0
'	If mdecl mdecl.ImportModule "monkey",0
	
	Repeat

		If line
			source.Push "~n"
			While toker.Toke And toker.Toke<>"~n" And toker.TokeType<>TOKE_LINECOMMENT
				toker.NextToke
			Wend
			If Not toker.Toke Exit
			toker.NextToke
		Endif
		line+=1
		
		_errInfo=toker.Path+"<"+toker.Line+">"
		
		If toker.TokeType=TOKE_SPACE toker.NextToke
		
		If toker.Toke<>"#"
			If cnest=ifnest
				Local line:=""
				While toker.Toke And toker.Toke<>"~n" And toker.TokeType<>TOKE_LINECOMMENT
				
					Local toke:=toker.Toke
					toker.NextToke
					
					If mdecl
						Select toke.ToLower()
						Case "public"
							attrs=0
						Case "private"
							attrs=DECL_PRIVATE
						Case "import"
							While toker.TokeType=TOKE_SPACE
								toke+=toker.Toke
								toker.NextToke
							Wend
							If toker.TokeType=TOKE_IDENT
								Local modpath:=toker.Toke
								While toker.NextToke="."
									modpath+="."
									toker.NextToke
									If toker.TokeType<>TOKE_IDENT Exit
									modpath+=toker.Toke
								Wend
								toke+=modpath
'								Print "Import found: "+toke
								mdecl.ImportModule modpath,attrs
							Endif
						End
					Endif
					
					line+=toke
				Wend
				If line source.Push line
			Endif
			Continue
		Endif
		
		Local toke:=toker.NextToke
		If toker.TokeType=TOKE_SPACE toke=toker.NextToke
		
		Local stm:=toke.ToLower()
		Local ty:=toker.TokeType()
		
		toker.NextToke
		
		If stm="end" Or stm="else"
			If toker.TokeType=TOKE_SPACE toker.NextToke
			If toker.Toke.ToLower()="if" 
				toker.NextToke
				stm+="if"
			Endif
		Endif
		
		Select stm
		Case "rem"
		
			ifnest+=1
			
		Case "if"
		
			ifnest+=1
		
			If cnest=ifnest-1
				If EvalBool( toker ) cnest=ifnest
			Endif
			
		Case "else"
		
			If Not ifnest Err "#Else without #If"
			
			If cnest=ifnest
				cnest|=$10000
			Else If cnest=ifnest-1
				cnest=ifnest
			Endif
			
		Case "elseif"
		
			If Not ifnest Err "#ElseIf without #If"
			
			If cnest=ifnest
				cnest|=$10000
			Else If cnest=ifnest-1
				If EvalBool( toker ) cnest=ifnest
			Endif
			
		Case "end","endif"
		
			If Not ifnest Err "#End without #If or #Rem"
			
			ifnest-=1
			
			If ifnest<(cnest & $ffff) cnest=ifnest
			
		Case "print"
		
			If cnest=ifnest
				Print EvalText( toker )
			Endif
			
		Case "error"
		
			If cnest=ifnest
				Err EvalText( toker )
			Endif

		Default
		
			If cnest=ifnest
				If ty=TOKE_IDENT
				
					If toker.TokeType=TOKE_SPACE toker.NextToke
					Local op:=toker.Toke()
					
					Select op
					Case "=","+="
					
						Select toke
						Case "HOST","LANG","CONFIG","TARGET","SAFEMODE"
							Err "App config var '"+toke+"' cannot be modified"
						End
						
						toker.NextToke
						
						Select op
						Case "="
							Local expr:=EvalExpr( toker )
							Local val:=expr.Eval()
							If Not GetConfigVars().Contains( toke )
								If StringType( expr.exprType ) val=EvalConfigTags( val )
								SetConfigVar toke,val,expr.exprType
							Endif
						Case "+="
							Local val:=EvalText( toker )
							Local var:=GetConfigVar( toke )
							If BoolType( GetConfigVarType( toke ) )
								If var="1" var="True" Else var="False"
							Endif
							If var And Not val.StartsWith( ";" ) val=";"+val
							SetConfigVar toke,var+val
						End
						
					Default
						Err "Expecting assignment operator."
					End
				Else
					Err "Unrecognized preprocessor directive '"+toke+"'"
				Endif

			Endif				
		End

	Forever

	SetConfigVar "MODPATH",p_modpath
	SetConfigVar "CD",p_cd

	PopEnv
		
	Return source.Join( "" )
End
