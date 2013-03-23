
Import parser

'Urgh, lost the plot a bit here...

Function Eval$( source$,ty:Type )

	Local env:=New ScopeDecl
	
	For Local kv:=Eachin _cfgVars
		env.InsertDecl New ConstDecl( kv.Key,0,Type.stringType,New ConstExpr( Type.stringType,kv.Value ) )
	Next

	PushEnv env
	
	Local toker:=New Toker( "",source )
	
	Local parser:=New Parser( toker,Null )

	Local expr:=parser.ParseExpr().Semant()
	
	Local val$
	
	If StringType( ty ) And BoolType( expr.exprType )
		val=expr.Eval()
		If val val="1" Else val="0"
	Else If BoolType( ty ) And StringType( expr.exprType )
		val=expr.Eval()
		If val And val<>"0" val="1" Else val="0"
	Else
		If ty expr=expr.Cast( ty )
		val=expr.Eval()
	Endif
	
	PopEnv
	
	Return val
End

Function Eval$( toker:Toker,type:Type )
	Local buf:=New StringStack
	While toker.Toke And toker.Toke<>"~n" And toker.TokeType<>TOKE_LINECOMMENT
		buf.Push toker.Toke
		toker.NextToke
	Wend
	Return Eval( buf.Join(""),type )
End

Function PreProcess$( path$ )

	Local cnest,ifnest,line,source:=New StringStack
	
	Local toker:=New Toker( path,LoadString( path ) )
	toker.NextToke
	
	SetCfgVar "CD",ExtractDir( RealPath( path ) )
	
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
				Local line$
				While toker.Toke And toker.Toke<>"~n" And toker.TokeType<>TOKE_LINECOMMENT
					Local toke$=toker.Toke
					line+=toke
					toker.NextToke
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
				If Eval( toker,Type.boolType ) cnest=ifnest
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
				If Eval( toker,Type.boolType ) cnest=ifnest
			Endif
			
		Case "end","endif"
		
			If Not ifnest Err "#End without #If or #Rem"
			
			ifnest-=1
			
			If ifnest<(cnest & $ffff) cnest=ifnest
			
		Case "print"
		
			If cnest=ifnest
				Print EvalCfgTags( Eval( toker,Type.stringType ) )
			Endif
			
		Case "error"
		
			If cnest=ifnest
				Err EvalCfgTags( Eval( toker,Type.stringType ) )
			Endif

		Default
		
			If cnest=ifnest
				If ty=TOKE_IDENT
				
					If toker.TokeType=TOKE_SPACE toker.NextToke
					Local op:=toker.Toke()
					
					If op="=" Or op="+="
					
						Select toke
						Case "HOST","LANG","CONFIG","TARGET","SAFEMODE"
							Err "App config var '"+toke+"' cannot be modified"
						End
						
						toker.NextToke
						
						Local val:=EvalCfgTags( Eval( toker,Type.stringType ) )
						
						If op="="
							If Not GetCfgVar( toke ) SetCfgVar toke,val
						Else If op="+="
							Local var:=GetCfgVar( toke )
							If var And Not val.StartsWith( ";" ) val=";"+val
							SetCfgVar toke,var+val
						Endif
					Else
						Err "Syntax error - expecting assignment"
					Endif
				Else
					Err "Unrecognized preprocessor directive '"+toke+"'"
				Endif

			Endif				
		End

	Forever
	
	SetCfgVar "CD",""
	
	Return source.Join( "" )
End
