
Import makedocs

Import parser

Class Decl

	Field kind:String
	Field ident:String
	Field type:String
	Field texts:String
	Field timpls:String
	Field scope:ScopeDecl
	Field path:String								'eg: mojo.graphics.DrawImage; monkey.list.List.AddLast
	Field uident:String								'unique identifier, eg: DrawImage(2)
	Field docs:=New StringMap<String>				'maps sections to docs
	Field egdir:String								'example data dir
	
	Method New( pdecl:parser.Decl,scope:ScopeDecl )
		'
		Self.kind=pdecl.kind
		Self.ident=pdecl.ident
		Self.type=pdecl.type
		Self.texts=pdecl.exts
		Self.timpls=pdecl.impls
		Self.scope=scope
		'
		path=ident
		uident=ident
		'
		If scope
			Local i:=1
			While scope.declsByUident.Contains( uident )
				i+=1
				uident=ident+"("+i+")"
			Wend
			path=scope.path+"."+uident
			scope.decls.Push Self
			scope.declsByUident.Set uident,Self
			scope.declsByKind.Get( kind ).Push Self
		Endif
		
		docs.Set "description",""
		docs.Set "example",""
		docs.Set "links",""
		docs.Set "params",""
		docs.Set "returns",""
		
	End
	
	Method PagePath:String()
		If ModuleDecl( Self )
			Return "Modules/"+uident
		Else If ScopeDecl( Self )
			Return scope.PagePath()+"/"+uident
		Else
			Return scope.PagePath()+"#"+uident
		Endif
	End
	
	Method FindDecl:Decl( path:String )
		Local decl:=FindDeclHere( path )
		If Not decl And scope decl=scope.FindDecl( path )
		Return decl
	End
	
	Method FindDeclHere:Decl( path:String )
		If path=uident Return Self
	End
	
End

Class ImportDecl Extends Decl

	Method New( pdecl:parser.Decl,scope:ScopeDecl )
		Super.New( pdecl,scope )
	End
	
	Method Resolve:ModuleDecl()
		Local doccer:=ModuleDecl( scope ).doccer
		Local mdecl:=ModuleDecl( doccer.scopes.Get( ident ) )
		If Not mdecl doccer.george.Err "Can't find import module: "+ident
		Return mdecl
	End
	
	Method PagePath:String()
		Local mdecl:=Resolve()
		If mdecl Return mdecl.PagePath()
		Return ""
	End
	
	Method FindDeclHere:Decl( path:String )
		Local mdecl:=Resolve()
		If mdecl Return mdecl.FindDeclHere( path )
		Return null
	End

End

Class ScopeDecl Extends Decl

	Field decls:=New Stack<Decl>					'inner decls
	Field declsByUident:=New StringMap<Decl>		'inner decls by uident
	Field declsByKind:=New StringMap<Stack<Decl>>	'inner decls by kind
	Field template:PageMaker

	Method New( pdecl:parser.Decl,scope:ScopeDecl )
		Super.New( pdecl,scope )
		'
		If scope template=scope.template
		'
		declsByKind.Set "ctor",New Stack<Decl>
		declsByKind.Set "framework",New Stack<Decl>
		declsByKind.Set "module",New Stack<Decl>
		declsByKind.Set "import",New Stack<Decl>
		declsByKind.Set "class",New Stack<Decl>
		declsByKind.Set "interface",New Stack<Decl>
		declsByKind.Set "function",New Stack<Decl>
		declsByKind.Set "method",New Stack<Decl>
		declsByKind.Set "property",New Stack<Decl>
		declsByKind.Set "global",New Stack<Decl>
		declsByKind.Set "field",New Stack<Decl>
		declsByKind.Set "const",New Stack<Decl>
		'
	End
	
	Method FindDeclHere:Decl( path:String )
		Local i:=path.Find( "." )
		If i=-1 Return declsByUident.Get( path )
		Local t:=declsByUident.Get( path[..i] )
		If t Return t.FindDeclHere( path[i+1..] )
		Return Null
	End
	
	Method SortDecls:Void()
		Local map:=New StringMap<Decl>
		For Local it:=Eachin declsByKind
			Local stack:=it.Value
			map.Clear
			For Local decl:=Eachin stack
				map.Set decl.uident,decl
			Next
			stack.Clear
			For Local it:=Eachin map
				stack.Push it.Value
			Next
		Next
	End
	
End

Class ClassDecl Extends ScopeDecl

	Field exts:ClassDecl							'extends
	Field impls:Stack<Decl>							'implements
	Field extby:=New Stack<ClassDecl>
	
	Method New( pdecl:parser.Decl,scope:ModuleDecl )
		Super.New( pdecl,scope )
	End	
	
	Method FindDeclHere:Decl( path:String )
		Local decl:=Super.FindDeclHere( path )
		If Not decl And exts decl=exts.FindDeclHere( path )
		Return decl
	End

End

Class ModuleDecl Extends ScopeDecl

	Field busy:Bool
	Field doccer:ApiDoccer
	
	Method New( pdecl:parser.Decl,doccer:ApiDoccer )
		Super.New( pdecl,Null )
		Self.doccer=doccer
		Self.template=doccer.scopeTemplate
	End	
	
	Method FindDeclHere:Decl( path:String )
		Local decl:=Super.FindDeclHere( path )
		If decl Or busy Return decl
		busy=True
		For Local imp:=Eachin declsByKind.Get( "import" )
			decl=imp.FindDeclHere( path )
			If decl Exit
		Next
		If Not decl decl=doccer.scopes.Get( "monkey.lang" ).FindDeclHere( path )
		busy=False
		Return decl
	End
End

Class ApiDoccer Implements ILinkResolver

	Field george:George
	Field scopes:=New StringMap<ScopeDecl>		'Maps path->scope, eg: brl.databuffer->ModuleDecl ; brl.databuffer.DataBuffer->ClassDecl
	Field scopeTemplate:PageMaker
	Field linkScope:ScopeDecl
	
	Method New( george:George )
	
		Self.george=george
	End
	
	Method ParseDocs:Void()

		ParseModules "modules",""
	End
	
	Method MakeDocs:Void()
	
		ResolveScopes
		
		Local template:=LoadString( george.styledir+"/scope_template.html" )
		
		Local maker:=New PageMaker( template )
		
		For Local decl:=Eachin scopes.Values
		
			MakeScopeDocs decl,maker
			
			Local page:=maker.MakePage()
			
			If decl.template
				decl.template.SetString( "CONTENT",page )
				page=decl.template.MakePage()
			Endif
			
			george.SetPageContent decl.PagePath(),page
		Next
		
	End
	
	Method Capitalize:String( str:String )
		Return str[0..1].ToUpper()+str[1..]
	End
	
	Method Pluralize:String( str:String )
		If str.EndsWith( "s" ) Return str+"es"
		If str.EndsWith( "y" ) Return str[..-1]+"ies"
		Return str+"s"
	End
	
	Method StripLinks:String( str:String )
		Return str.Replace( "[[","" ).Replace( "]]","" )
	End
	
	Method HtmlEsc:String( str:String )
		Return str.Replace( "&","&amp;" ).Replace( "<","&lt;" ).Replace( ">","&gt;" )
	End
	
	Method ResolveLink:String( link:String,text:String )

		'Kludge! One char uppercase links are *probably* template args!	
		If link.Length=1 And link=link.ToUpper() Return link
		
		Local decl:=linkScope.FindDecl( link )
		If decl Return george.ResolveLink( decl.PagePath(),text )
		Return george.ResolveLink( link,text )
	End
	
	Method AddDecl:Void( decl:Decl,maker:PageMaker,markdown:Markdown,prefix:String )
		maker.SetString prefix+"KIND",Capitalize( decl.kind )
		maker.SetString prefix+"IDENT",decl.ident
		maker.SetString prefix+"UIDENT",decl.uident
		maker.SetString prefix+"URL",george.GetPageUrl( decl.PagePath() )
		maker.SetString prefix+"PATH",decl.path
		maker.SetString prefix+"TYPE",StripLinks( HtmlEsc( decl.type ) )
		maker.SetString prefix+"XTYPE",markdown.ToHtml( HtmlEsc( decl.type ) )
		maker.SetString prefix+"EXTENDS",StripLinks( HtmlEsc( decl.texts ) )
		maker.SetString prefix+"XEXTENDS",markdown.ToHtml( HtmlEsc( decl.texts ) )
		maker.SetString prefix+"IMPLEMENTS",StripLinks( HtmlEsc( decl.timpls ) )
		maker.SetString prefix+"XIMPLEMENTS",markdown.ToHtml( HtmlEsc( decl.timpls ) )
		
		Local cdecl:=ClassDecl( decl )
		If cdecl And Not cdecl.extby.IsEmpty()
			Local extby:="",xextby:=""
			For Local decl:=Eachin cdecl.extby
				If extby extby+=", "
				If xextby xextby+=", "
				extby+=decl.ident
				xextby+=george.ResolveLink( decl.PagePath(),decl.ident )
			Next
			maker.SetString prefix+"EXTENDED_BY",extby
			maker.SetString prefix+"XEXTENDED_BY",xextby
		Endif
		
		'fix example
		Local eg:=decl.docs.Get( "example" )
		If eg
			eg=eg.Trim()
			If eg.StartsWith( "<pre>" ) eg=eg[5..]
			If eg.EndsWith( "</pre>" ) eg=eg[..-6]
			decl.docs.Set "example","<pre>"+eg+"</pre>"
			
			'save example
			Local file:=decl.path.Replace( ".","_" )+".monkey"
			SaveString eg,"docs/html/examples/"+file
			decl.docs.Set "EXAMPLE_URL","examples/"+file
			If FileType( decl.egdir )=FILETYPE_DIR
				CopyDir decl.egdir,"docs/html/examples/"+StripExt( file )+".data"
			Endif
			
		Endif
		
		'write docs
		For Local it:=Eachin decl.docs
			Local html:=it.Value
			If html html=markdown.ToHtml( it.Value )
			maker.SetString prefix+it.Key.ToUpper(),html
		Next
		
	End
	
	Method ResolveScopes:Void()
		For Local it:=Eachin scopes
		
			Local scope:=it.Value
			
			george.SetErrInfo scope.path
			
			linkScope=scope

			Local cdecl:=ClassDecl( scope )
			If cdecl And cdecl.texts
				Local i:=cdecl.texts.Find( "]]" )
				If i<>-1 
					Local exts:=ClassDecl( cdecl.scope.FindDecl( cdecl.texts[2..i] ) )
					If exts
						cdecl.exts=exts
						exts.extby.Push cdecl
					Endif
				Endif
				If Not cdecl.exts george.Err "Can't find super class: "+cdecl.texts
			Endif
		Next
	
		linkScope=Null
		
		george.SetErrInfo ""
	End
	
	Method MakeScopeDocs:Void( scope:ScopeDecl,maker:PageMaker )

		george.SetErrInfo scope.path
		
		linkScope=scope
		Local markdown:=New Markdown( Self,george )

		'create summary from desc.		
		Local desc:=scope.docs.Get( "description" )
		If desc
			Local summary:=desc
			Local i:=desc.Find( "." )
			If i<>-1
				i+=1
				While i<desc.Length And desc[i]>32
					i+=1
				Wend
				summary=desc[..i]
			Endif
			scope.docs.Set "summary",summary
			If summary=desc
				If Not scope.docs.Get( "example" ) And Not scope.docs.Get( "links" ) scope.docs.Set "description",""
			Endif
		Endif
		
		maker.Clear
		AddDecl scope,maker,markdown,""
		
		Local kinds:=["class","function","method","property","global","field","const","import","interface","ctor"]
		
		scope.SortDecls
		
		For Local kind:=Eachin kinds
			Local decls:=scope.declsByKind.Get( kind )
			If Not decls.Length Continue
			
			maker.BeginList Pluralize(kind).ToUpper()
			For Local decl:=Eachin decls
				maker.AddItem
				AddDecl decl,maker,markdown,""
			Next
			maker.EndList
		Next
		
		linkScope=Null
		
		george.SetErrInfo ""

	End
	
	Method EndSect:Void( sect:String,docs:StringStack,doccing:Decl )
		If Not docs.IsEmpty()
			Local t:=docs.Join( "~n" ).Trim()
			If t doccing.docs.Set sect,t
			docs.Clear
		Endif
	End
	
	
	Method AddDocsToDecl:Void( docs:StringMap<StringStack>,decl:Decl )
		If Not docs Return
		For Local it:=Eachin docs
			decl.docs.Set it.Key,it.Value.Join( "~n" )
		Next
	End
	
	Method LoadExample:Bool( decl:Decl,dir:String )
		If Not dir Or decl.ident<>decl.uident Return False
		Local src:=LoadString( dir+"/"+decl.ident+"_example.monkey" )
		If Not src Return False
		decl.docs.Set "example",src
		decl.egdir=dir+"/"+decl.ident+"_example.data"
		Return True
	End
	
	Method ParseMonkeyFile:Void( srcpath:String,modpath:String )
	
		george.SetErrInfo srcpath
		
		Local parser:=New Parser( "" )
		
		Local mdecl:ModuleDecl
		
		Local docscope:ScopeDecl
		Local docs:StringMap<StringStack>
		Local sect:String
		
		Local pub:=True
		
		Local egdir:=ExtractDir( srcpath )+"/examples"
		If FileType( egdir )<>FILETYPE_DIR egdir=""

		Local src:=LoadString( srcpath ).Replace( "~r","" )
		
		For Local line:=Eachin src.Split( "~n" )
		
			parser.SetText line

			If parser.Toke="#"
				Select parser.Bump()
				Case "rem"
					If parser.Bump()="monkeydoc"
						If Not mdecl 
							If parser.Bump()<>"module" Return
							Local id:=parser.Bump()
							If id<>modpath
								george.Err "Modpath ("+modpath+") does not match module ident ("+id+")"
								Return
							Endif
						Endif
						docs=New StringMap<StringStack>
						sect="description"
						docs.Set sect,New StringStack
						Local text:=parser.GetText().Trim()
						If text docs.Get( sect ).Push text
					Endif
				Case "end"
					If sect
						If Not mdecl
							mdecl=New ModuleDecl( New parser.Decl( "module",modpath ),Self )
							scopes.Set mdecl.path,mdecl
							george.AddPage mdecl.PagePath()
							docscope=mdecl
							AddDocsToDecl docs,mdecl
							docs=Null
							
							LoadExample mdecl,egdir
						Endif
						sect=""
					Endif
				End
				Continue
			Endif
			
			If sect
				If parser.TokeType=Toker.Identifier
					Local id:=parser.Toke
					If parser.Bump=":"
						Select id.ToLower()
						Case "params","returns","in","out","example","links"
							sect=id.ToLower()
							docs.Set sect,New StringStack
							Local text:=parser.GetText().Trim()
							If text docs.Get( sect ).Push text
							Continue
						End
					Endif
				Endif
				docs.Get( sect ).Push line
				Continue
			Endif
			
			If line.Trim() And Not mdecl Return
			
			Select parser.Toke
			Case "public"
				pub=True
			Case "private"
				pub=False
			Case "extern"
				pub=parser.Bump()<>"private"
			Case "import"
				If pub
					Local pdecl:=parser.ParseDecl()
					If pdecl New ImportDecl( pdecl,mdecl )
				Endif
			Case "class","interface"
				If pub Or docs
				
					Local cdecl:=New ClassDecl( parser.ParseDecl(),mdecl )
					scopes.Set cdecl.path,cdecl
					AddDocsToDecl docs,cdecl
					docscope=cdecl
					
					george.AddPage cdecl.PagePath()
					If cdecl.kind="class" george.AddToIndex "Classes",cdecl.ident,cdecl.PagePath()
					If cdecl.kind="interface" george.AddToIndex "Interfaces",cdecl.ident,cdecl.PagePath()
					
					LoadExample cdecl,egdir
				Endif
				docs=Null
			Case "function","method","global","field","const","property","ctor"
				If pub Or docs
				
					Local decl:=New Decl( parser.ParseDecl(),docscope )
					AddDocsToDecl docs,decl
					
					george.AddPage decl.PagePath()
					If decl.kind="function" And docscope=mdecl george.AddToIndex "Functions",decl.ident,decl.PagePath()
					
					LoadExample decl,egdir
				Endif
				docs=Null
			End

		Next
		
		george.SetErrInfo ""
	
	End
	
	Method ParseMonkeydocFile:Void( srcpath:String,modpath:String )
	
		george.SetErrInfo srcpath
		
		Local parser:=New Parser( "" )
		Local pdecl:=New parser.Decl( "module" )
		pdecl.ident=modpath
	
		Local mdecl:=New ModuleDecl( pdecl,Self )
		scopes.Set mdecl.path,mdecl
		
		george.AddPage mdecl.PagePath()
'		george.AddToIndex "Index",mdecl.path,mdecl.PagePath()
		
		Local scope:ScopeDecl=mdecl
		Local sect:="description"
		Local docs:=New StringStack
		Local doccing:Decl=mdecl
		
		Local egdir:=ExtractDir( srcpath )+"/examples"
		If FileType( egdir )<>FILETYPE_DIR And StripDir( ExtractDir( srcpath ) )="monkeydoc"
			egdir=ExtractDir( ExtractDir( srcpath ) )+"/examples"
			If FileType( egdir )<>FILETYPE_DIR egdir=""
		Endif

		LoadExample doccing,egdir

		Local src:=LoadString( srcpath )
				
		For Local line:=Eachin src.Split( "~n" )
		
			If line.StartsWith( "# " )
			
				parser.SetText line[2..] 
				Local pdecl:=parser.ParseDecl()
				If Not pdecl
					george.Err "Error parsing line: "+line
					Continue
				Endif
				
				Select pdecl.kind
				Case "module"
				
					EndSect sect,docs,doccing
					sect="description"
					scope=mdecl
					doccing=mdecl
					
				Case "import"
				
					If scope<>mdecl george.Err "Import not at Module scope"
					New ImportDecl( pdecl,mdecl )
					
				Case "class","interface"

					If pdecl.ident.Contains( "." ) 
						pdecl.ident=ExtractExt( pdecl.ident )
					Endif

					EndSect sect,docs,doccing
					sect="description"

					Local cdecl:=New ClassDecl( pdecl,mdecl )
					scopes.Set cdecl.path,cdecl
					scope=cdecl
					doccing=scope
					
					george.AddPage cdecl.PagePath()
					If cdecl.kind="class" george.AddToIndex "Classes",cdecl.ident,cdecl.PagePath()
					If cdecl.kind="interface" george.AddToIndex "Interfaces",cdecl.ident,cdecl.PagePath()

					LoadExample doccing,egdir
					
				Case "function","method","global","field","const","property","ctor"
				
					EndSect sect,docs,doccing
					sect="description"

					doccing=New Decl( pdecl,scope )
					
					george.AddPage doccing.PagePath()
					If doccing.kind="function" And scope=mdecl george.AddToIndex "Functions",doccing.ident,doccing.PagePath()

					LoadExample doccing,egdir

				 Default
				 	george.Err "Unrecognized decl kind: "+pdecl.kind
				End
				
			Else If line.StartsWith( "'# " )
			
			Else
				parser.SetText line
				If parser.TokeType=Toker.Identifier
					Local id:=parser.ParseIdent()
					If parser.Toke=":"
						Select id.ToLower()
						Case "params","returns","in","out","example","links"
							EndSect sect,docs,doccing
							sect=id.ToLower()
							Local toker:=parser.GetToker()
							Local t:=toker.Text[toker.Cursor..].Trim()
							If t docs.Push t
							Continue
						End
					Endif
				Endif
				docs.Push line
			End
		Next
		
		EndSect sect,docs,doccing
		
		george.SetErrInfo ""
	End
	
	Method ParseModules:Void( dir:String,modpath:String )
	
		Local tmp:=scopeTemplate
		
		Local p:=dir+"/scope_template.html"
		If FileType( p )=FILETYPE_FILE 
			scopeTemplate=New PageMaker( LoadString( p ) )
		Endif
		
		For Local f:=Eachin LoadDir( dir )
			Local p:=dir+"/"+f
			Select FileType( p )
			Case FILETYPE_DIR
				If f="brlx" Continue
				If modpath
					ParseModules p,modpath+"."+f
				Else
					ParseModules p,f
				Endif
			Case FILETYPE_FILE
				Local name:=StripExt( f )
				Local ext:=ExtractExt( f )
				If ext="monkey"
					Local q:=modpath
					If name<>StripDir( dir ) q+="."+name
					
					Local t:=dir+"/"+name+".monkeydoc"
					If FileType( t )=FILETYPE_FILE
						ParseMonkeydocFile t,q
						Continue
					Endif
					
					t=dir+"/monkeydoc/"+name+".monkeydoc"
					If FileType( t )=FILETYPE_FILE
						ParseMonkeydocFile t,q
						Continue
					Endif

					ParseMonkeyFile p,q

				Endif
			End
			
		Next

		scopeTemplate=tmp
	End
End
