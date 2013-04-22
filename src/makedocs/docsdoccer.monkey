
Import makedocs

Class DocsDoccer Implements ILinkResolver

	Field george:George
	
	Field docs:=New StringMap<String>
	
	Method New( george:George )
		Self.george=george
	End
	
	Method ResolveLink:String( link:String,text:String )
	
		If link.StartsWith( "../" ) And link.EndsWith( ".monkey" )
			If Not text text=StripDir( link )
			Return george.MakeLink( link,text )
		Endif
		
		Return george.ResolveLink( link,text )
	End
	
	Method ParseDocs:Void( dir:String,indexcat:String )
	
		For Local f:=Eachin LoadDir( dir,True )
		
			If ExtractExt( f )<>"monkeydoc" Continue
			
			Local docpath:=StripExt( f )
			
			george.AddPage docpath
			
			docs.Set docpath,dir+"/"+f
		Next
	End
	
	Method ParseDocs:Void()
	
		ParseDocs "docs/monkeydoc",""

	End
	
	Method MakeDocs:Void()
	
		Local markdown:=New Markdown( Self,george )
			
		For Local it:=Eachin docs
		
			george.SetDocBase it.Key

			george.SetErrInfo it.Value
			
			Local src:=LoadString( it.Value )
			
			george.SetPageContent it.Key,markdown.ToHtml( src )
			
		Next
		
		george.SetDocBase ""
	End

End
