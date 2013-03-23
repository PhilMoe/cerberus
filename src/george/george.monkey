
#If TARGET<>"stdcpp"
#Error "TARGET must be stdcpp"
#End

Import parser
Import page
Import markdown

Class Docs Implements LinkResolver

	Method New( template:String )
		_template=template	
	End
	
	Method MakeDocs:Void()
	
		For Local f:=Eachin LoadDir( "src_docs" )
		
			If f.StartsWith(".") Or ExtractExt(f)<>"txt" Continue
			
			f=StripExt(f)
			
			If f.StartsWith("_")
				LoadScope f[1..]
			Else
				LoadTopic f
			Endif
		Next
		
		_topics.Set "Module list",""
		_topics.Set "Function index",""
		_topics.Set "Class index",""

		MakeScopes
		MakeTopics
		MakeIndices
	End
	
	Private
	
	'page template
	Field _template:String
	
	'maps paths to decls
	Field _declsByPath:=New StringMap<Decl>
	
	'maps idents to decls
	Field _declsByIdent:=New StringMap<Decl>
	
	'for link resolver
	Field _linkScope:Decl

	'for non-decls pages
	Field _topics:=New StringMap<String>
	
	Method Err( msg:String="Docs error" )
		Error msg
	End
	
	Method HtmlEsc:String( str:String )
		Return str.Replace("&","&amp;").Replace("<","&lt;").Replace(">","&gt;")
	End
	
	Method AddDecl:Void( decl:Decl,id:String,map:StringMap<Decl>,notctors:Bool=True )
	
		If notctors And decl.kind="ctor" Return
		
		Local t:=id,n:=1

		If map.Contains(t)
			Local d:=map.Get(t)
			If decl.path<>d.path
				'globals have priority over members...
				Local pri:=False
				Select decl.kind
				Case "module","class","interface"
					pri=True
				Case "function","global","const"
					Select DeclScope(decl).kind
					Case "module","class","interface"
						pri=True
					End
				End
				If pri
					map.Set t,decl
					decl=d
				Endif
			Endif
			Repeat
				n+=1
				t=id+" ("+n+")"
			Until Not map.Contains(t)
		Endif
		map.Set t,decl
	End
	
	Method DeclScope:Decl( decl:Decl )
		Local path:=decl.path
		Local i:=path.FindLast(".")
		If i<>-1 Return _declsByPath.Get(path[..i])
		Return Null
	End

	Method DeclPath:String( decl:Decl )
		Return decl.path
	End
	
	Method DeclIdent:String( decl:Decl )
		Return decl.ident
	End

	Method DeclUrl:String( decl:Decl )
		Select decl.kind
		Case "module","class","interface","import","import p"
			Return "_"+DeclPath(decl)+".html"
		Case "const","global","field","ctor","prop","method","function"
			Return DeclUrl(DeclScope(decl))+"#"+DeclIdent(decl)
		End
		Err
	End
	
	Method FindDecl:Decl( link:String,scope:Decl )
		If scope
			Local sc:=DeclPath(scope)
			While sc
				Local d:=_declsByPath.Get(sc+"."+link)
				If d Return d
				Local i:=sc.FindLast(".")
				If i=-1 Exit
				sc=sc[..i]
			Wend
		Endif
		If link.Contains(".") And _declsByPath.Contains(link) Return _declsByPath.Get(link)
		Return _declsByIdent.Get(link)
	End
	
	Method ResolveLink:String( link:String,text:String )
	
		Local url:=""
		
		If link.Contains( "/" )
			url=link
		Else If link.StartsWith("#")
			url=link
		Else
			If link.Length=1 Return text

			Local id:=""
			Local i:=link.Find("#")
			If i<>-1
				id=link[i+1..]
				link=link[..i]
			Endif

			'underscore confusion!			
			If link="Language_reference" link="Language reference"
			
			Local d:=FindDecl(link,_linkScope)
			If Not d And _linkScope
				For Local imp:=Eachin _linkScope.decls
					If Not imp.kind.StartsWith("import") Continue
					d=FindDecl(link,imp)
					If d Exit
				Next
			Endif
			
			If d
				url=DeclUrl(d)
			Else If _topics.Contains(link)
				url=link+".html"
			Else
				If _linkScope
					Print "LinkAnchor: Can't resolve wiki link:"+link+" in scope: "+_linkScope.ident
				Else
					Print "LinkAnchor: Can't resolve wiki link:"+link
				Endif
				Return text
			Endif
			If id url+="#"+id
		Endif
		
		Return "<a href='"+url+"'>"+text+"</a>"
		
	End	

	Method SavePage:Void( page:Page,path:String,prefix:String="" )

		Local url:=""
		page.BeginList "NAVLINKS"
		If path<>"Home"
			For Local bit:=Eachin path.Split(".")
				If url url+="."
				url+=bit
				page.AddItem
				page.Set "URL",prefix+url+".html"
				page.Set "TEXT",bit
			Next
		Endif
		page.EndList
		
		Local out:=page.GeneratePage()
		SaveString out,"../../docs/html/"+prefix+path+".html"
	End
	
	Method MakeType:String( type:String,markdown:Markdown )
		If type Return type.Replace("[[","").Replace("]]","")
		Return ""
	End
	
	Method MakeXType:String( type:String,markdown:Markdown )
		If type Return markdown.ToHtml(type)
		Return ""
	End

	Method MakeScope:Void( scope:Decl )

		Local page:=New Page(_template)

		_linkScope=scope
		Local markdown:=New Markdown( Self )

		page.Set "SCOPE",scope.kind[..1].ToUpper()+scope.kind[1..]
		page.Set "PATH",DeclPath(scope)
		page.Set "IDENT",DeclIdent(scope)
		
		Select scope.kind
		Case "class","interface"
		
			page.Set "ARGS",HtmlEsc(scope.args)
			page.Set "TYPE",MakeType(scope.type,markdown)
			page.Set "XTYPE",MakeXType(scope.type,markdown)
			
			Local ty:=scope.type
			Local i:=ty.Find("Implements")
			If i<>-1
				Local impls:=ty[i+10..].Trim()
				page.Set "IMPLEMENTS",MakeType(impls,markdown)
				page.Set "XIMPLEMENTS",MakeXType(impls,markdown)
				ty=ty[..i]
			Endif
			i=ty.Find("Extends")
			If i<>-1
				Local exts:=ty[i+7..].Trim()
				page.Set "EXTENDS",MakeType(exts,markdown)
				page.Set "XEXTENDS",MakeXType(exts,markdown)
			Endif
		End
		
		Local short_desc:="",long_desc:=""
		Local desc:=scope.docs.Get("description")
		If desc
			Local i:=0
			While i<desc.Length
				i=desc.Find(".",i)+1
				If i=0 Or i=desc.Length Or desc[i]<=32 Exit
			Wend
			If i>0 And desc[i..].Trim()
				short_desc=desc[..i]
				long_desc=desc
			Else
				short_desc=desc
				long_desc=""
			Endif
		Endif
		If short_desc page.Set "SHORT_DESC",markdown.ToHtml(short_desc)
		If long_desc page.Set "LONG_DESC",markdown.ToHtml(long_desc)
		
		'imports...
		Local imps:=New StringMap<Decl>
		For Local decl:=Eachin scope.decls
			If decl.kind<>"import" Continue
			imps.Set DeclIdent(decl),decl
		Next
		page.BeginList "IMPORTS"
		For Local it:=Eachin imps
			page.AddItem
			page.Set "IDENT",it.Key
			page.Set "PATH",DeclPath(it.Value)
			page.Set "URL",DeclUrl(it.Value)
		Next
		page.EndList
		
		'classes...
		Local classes:=New StringMap<Decl>
		For Local decl:=Eachin _declsByPath.Values()
			If decl.kind<>"class" Or DeclScope(decl)<>scope Continue
			classes.Set DeclIdent(decl),decl
		Next
		page.BeginList "CLASSES"
		For Local it:=Eachin classes
			page.AddItem
			page.Set "IDENT",it.Key
			page.Set "PATH",DeclPath(it.Value)
			page.Set "URL",DeclUrl(it.Value)
		Next
		page.EndList
		
		'interfaces...
		Local ifaces:=New StringMap<Decl>
		For Local decl:=Eachin _declsByPath.Values()
			If decl.kind<>"interface" Or DeclScope(decl)<>scope Continue
			ifaces.Set DeclIdent(decl),decl
		Next
		page.BeginList "IFACES"
		For Local it:=Eachin ifaces
			page.AddItem
			page.Set "IDENT",it.Key
			page.Set "PATH",DeclPath(it.Value)
			page.Set "URL",DeclUrl(it.Value)
		Next
		page.EndList
		
		'maps decl.kind -> sorted map
		Local sorted:=New StringMap<StringMap<Decl>>

		For Local decl:=Eachin scope.decls
		
			If decl.kind.StartsWith("import") Continue
			
			Local map:=sorted.Get(decl.kind)
			If Not map
				map=New StringMap<Decl>
				sorted.Set decl.kind,map
			Endif
			
			AddDecl decl,DeclIdent(decl),map,False
		Next
	
		Local kinds:=["const","global","field","ctor","prop","method","function"]
		
		For Local kind:=Eachin kinds
		
			Local map:=sorted.Get(kind)
			If Not map Continue
			
			Local t:=kind.ToUpper()
			If t.EndsWith("S") t+="ES" Else t+="S"
			
			page.BeginList t
			
			For Local it:=Eachin map
			
				Local decl:=it.Value
				
				page.AddItem
				page.Set "PATH",DeclPath(decl)
				page.Set "IDENT",DeclIdent(decl)
				page.Set "UIDENT",it.Key
				page.Set "TYPE",MakeType(decl.type,markdown)
				page.Set "XTYPE",MakeXType(decl.type,markdown)
				page.Set "ARGS",MakeType(decl.args,markdown)
				page.Set "XARGS",MakeXType(decl.args,markdown)
				
				For Local it:=Eachin decl.docs
					page.Set it.Key.Replace(" ","_").ToUpper(),markdown.ToHtml( it.Value )
				Next
				
			Next
			
			page.EndList
			
		End
		SavePage page,DeclPath(scope),"_"
	End
	
	Method MakeScopes:Void()	
		For Local it:=Eachin _declsByPath
			Local decl:=it.Value
			If decl.kind<>"module" And decl.kind<>"class" And decl.kind<>"interface" Continue
			Print "Making scope:"+it.Key
			MakeScope decl
		Next
	End
	
	Method MakeTopics:Void()
		_linkScope=Null
		Local markdown:=New Markdown(Self)
		For Local it:=Eachin _topics
'			Print "Making page:"+it.Key
			Local page:=New Page(_template)
			page.Set "TOPIC",markdown.ToHtml(it.Key)
			page.Set "CONTENT",markdown.ToHtml(it.Value)
			SavePage page,it.Key,""
		Next
	End
	
	Method MakeIndices:Void()

		Local page:Page
		
		'modules list
		page=New Page(_template)
		page.Set "LIST","Module list"
		page.BeginList "ITEMS"
		For Local it:=Eachin _declsByPath
			Local decl:=it.Value
			If decl.kind<>"module" Continue
			page.AddItem
			page.Set "TEXT",decl.path
			page.Set "URL",DeclUrl(decl)
		Next
		page.EndList
		SavePage page,"Module list"

		'class index	
		page=New Page(_template)
		page.Set "INDEX","Class index"
		page.BeginList "ITEMS"
		For Local it:=Eachin _declsByIdent
			Local decl:=it.Value
			If decl.kind<>"class" Continue
			page.AddItem
			page.Set "TEXT",it.Key
			page.Set "URL",DeclUrl(decl)
		Next
		page.EndList
		SavePage page,"Class index"

		page=New Page(_template)
		page.Set "INDEX","Interface index"
		page.BeginList "ITEMS"
		For Local it:=Eachin _declsByIdent
			Local decl:=it.Value
			If decl.kind<>"interface" Continue
			page.AddItem
			page.Set "TEXT",it.Key
			page.Set "URL",DeclUrl(decl)
		Next
		page.EndList
		SavePage page,"Interface index"

		'function index
		page=New Page(_template)
		page.Set "INDEX","Function index"
		page.BeginList "ITEMS"
		For Local it:=Eachin _declsByIdent
			Local decl:=it.Value
			If decl.kind<>"function" Continue
			Local sc:=DeclScope(decl)
			If Not sc Or sc.kind<>"module" Continue
			page.AddItem
			page.Set "TEXT",it.Key
			page.Set "URL",DeclUrl(decl)
		Next
		page.EndList
		SavePage page,"Function index"
		
		'index
		Local index:=New StringMap<String>
		For Local it:=Eachin _declsByIdent
			index.Set it.Key,DeclUrl(it.Value)
		Next
		For Local it:=Eachin _topics
			If index.Contains(it.Key) Err "Duplicate index entry"
			index.Set it.Key,it.Key+".html"
		Next
		
		'html index
		page=New Page(_template)
		page.Set "INDEX","Index"
		page.BeginList "ITEMS"
		For Local it:=Eachin index
			page.AddItem
			page.Set "TEXT",it.Key
			page.Set "URL",it.Value
		Next
		page.EndList
		SavePage page,"Index"

		'.txt index
		Local buf:=New StringStack
		For Local it:=Eachin index
'			If it.Key="Demo" Continue
			buf.Push it.Key+":"+it.Value
		Next
		SaveString buf.Join("~n"),"../../docs/html/index.txt"
		
	End

	Method LoadTopic:Void( path:String )
	
		Local src:=LoadString( "src_docs/"+path+".txt" )
		If Not src
			Print "Can't load: "+path
			Return
		End
		
		_topics.Set path,src
	End
	
	Method LoadScope:Void( path:String )
	
		Print "Loading: "+path
	
		Local src:=LoadString("src_docs/_"+path+".txt")
		If Not src
			Print "Can't load: "+path
			Return
		End
		
		src=src.Replace("~r","")
		
		Local decl:Decl,scope:Decl,decls:=New Stack<Decl>
		Local lines:=src.Split("~n"),block:="",buf:=New StringStack
		
		For Local line:=Eachin lines
		
			If line.StartsWith("'# ") Continue
		
			If line.StartsWith("# ")
			
				If Not buf.IsEmpty()
					decl.docs.Set block,buf.Join("~n").Trim()
					buf.Clear
				Endif
			
				Local p:=New Parser(line[2..])

				decl=p.ParseDecl()
				
				Select decl.kind
				Case "module","class","interface"
					If Not decls.IsEmpty()
						scope.decls=decls.ToArray()
						decls.Clear
					Endif
					decl.path=decl.ident
					Local i:=decl.path.Find("<")
					If i<>-1 decl.path=decl.path[..i]
					i=decl.path.FindLast(".")
					If i<>-1 decl.ident=decl.ident[i+1..]
					If _declsByPath.Contains(decl.path) Err "Duplicate decl:"+decl.ident
					scope=decl
				Case "import","import p"
					If Not scope Err
					decl.path=decl.ident
					Local i:=decl.ident.FindLast(".")
					If i<>-1 decl.ident=decl.ident[i+1..]
					decls.Push decl
					decl=scope
					Continue
				Case "const","global","field","ctor","prop","method","function"
				
					If Not scope Err
					decl.path=scope.path+"."+decl.ident
					decls.Push decl
				Default
					Err
				End
				
				AddDecl decl,DeclPath(decl),_declsByPath
				
				AddDecl decl,DeclIdent(decl),_declsByIdent
				
				block="description"
				
			Else If line.StartsWith("## ")
				If Not decl Err
				
				If Not buf.IsEmpty()
					decl.docs.Set block,buf.Join("~n").Trim()
					buf.Clear
				Endif
				
				block=line[3..].Trim()
				
			Else If block
			
				buf.Push line
				
			Endif
		
		Next
		
		If Not buf.IsEmpty()
			decl.docs.Set block,buf.Join("~n").Trim()
			buf.Clear
		Endif
		
		If Not decls.IsEmpty()
			scope.decls=decls.ToArray()
			decls.Clear
		Endif
		
	End
	
End

Function Main()

	ChangeDir "../../"
	
	Local docs:=New Docs(LoadString("page_template.html"))
	
	docs.MakeDocs

End
