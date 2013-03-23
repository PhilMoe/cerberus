
Interface LinkResolver

	Method ResolveLink:String( link:String,text:String )

End

Class Markdown

	Method New( resolver:LinkResolver )
		_resolver=resolver
	End

	Method ToHtml:String( src:String )
	
		If Not src.Contains("~n") Return LineToHtml(src)
		
		Local buf:=New StringStack
		For Local line:=Eachin src.Split("~n")
			buf.Push LineToHtml(line)
		Next
		
		Return buf.Join("~n")
	End

	Private

	Field _pre:Bool	
	Field _resolver:LinkResolver

	Method Find:Int( src:String,text:String,start:Int )
		Local i:=src.Find(text,start)
		If i=-1 Or (i>0 And src[i-1]=92) Return -1	'ESC with \
		Return i
	End
	
	Method ReplaceLinks:String( src:String )
	
		'
		'[[link]]  [[link|text]]
		Local i:=0
		Repeat

			Local i0:=Find(src,"[[",i)
			If i0=-1 Exit
			Local i1:=Find(src,"]]",i0+2)
			If i1=-1 Exit
			
			Local t:=src[i0+2..i1],p:=t
			Local j:=t.Find("|")
			If j<>-1
				p=t[..j]
				t=t[j+1..]
			Endif
	
			Local r:=_resolver.ResolveLink(p,t)
			
			src=src[..i0]+r+src[i1+2..]
			i=i0+r.Length
		Forever
		'
		'[text](link)
		i=0
		Repeat
			Local i0:=Find(src,"[",i)
			If i0=-1 Exit
			Local i1:=Find(src,"](",i0+1)
			If i1=-1 Or i1=i0+1 Exit
			Local i2:=Find(src,")",i1+2)
			If i2=-1 Or i2=i1+2 Exit
			'
			Local t:=src[i0+1..i1]
			Local p:=src[i1+2..i2]
			'
			Local r:=_resolver.ResolveLink(p,t)
			src=src[..i0]+r+src[i2+1..]
			i=i0+r.Length
		Forever
		'
		Return src
	End
	
	Method ReplaceTags:String( src:String,tag:String,html:String )
		'		
		'tag...tag -> <html>...</html>
		Local i:=0,l:=tag.Length
		Repeat
			Local i0:=Find(src,tag,i)
			If i0=-1 Exit
			Local i1:=Find(src,tag,i0+l)
			If i1=-1 Or i1=i0+l Exit
			Local r:="<"+html+">"+src[i0+l..i1]+"</"+html+">"
			src=src[..i0]+r+src[i1+l..]
			i=i0+r.Length
		Forever
		Return src
	End
	
	Method ReplaceTags2:String( src:String,tag:String,html:String )
		'
		'tag ident... -> <html>ident</html>
		Local i:=0,l:=tag.Length
		Repeat
			Local i0:=Find(src,tag,i)
			If i0=-1 Exit
			Local i1:=i0+l
			While i1<src.Length
				Local c:=src[i1]
				If (c=95) Or (c>=65 And c<=90) Or (c>=97 And c<=122) Or  (i1>i0+l And c>=48 And c<=57) 
					i1+=1
					Continue
				Endif
				Exit
			Wend
			'find
			If i1=i0+l
				i+=l
				Continue
			Endif
			Local r:="<"+html+">"+src[i0+l..i1]+"</"+html+">"
			src=src[..i0]+r+src[i1..]
			i=i0+r.Length
		Forever
		Return src		
	End
	
	Method ReplaceEscs:String( src:String )
		'\char -> char
		Local i:=0
		Repeat
			Local i0:=src.Find("\",i)
			If i0=-1 Exit
			Local r:=src[i0+1..i0+2]
			Select r
			Case "<" r="&lt;"
			Case ">" r="&gt;"
			Case "&" r="&amp;"
			End
			src=src[..i0]+r+src[i0+2..]
			i=i0+r.Length
		Forever
		Return src
	End
	
	Method LineToHtml:String( src:String )

		'dodgy hack for blank lines in <pre>
		If Not src And Not _pre Return "<p>"
		If src.StartsWith( "<pre>" ) _pre=True Else If src.StartsWith( "</pre>" ) _pre=False
		
		If src.StartsWith("#")
			Local i:=1
			While i<src.Length And src[i]=35	'"#"[0]
				i+=1
			Wend
			src=LineToHtml(src[i..])
			Return "<h"+i+">"+src+"</h"+i+">"
		Endif
		
		If src.StartsWith("*") And src.Length>1 And src[1]<=32
			Return "<li>"+LineToHtml(src[2..])+"</li>"
		Endif
		
		If src.StartsWith("+") And src.Length>1 And src[1]<=32
			Return "<li>"+LineToHtml(src[2..])+"</li>"
		Endif

		src=ReplaceLinks(src)

		src=ReplaceTags(src,"`","code")
		
		src=ReplaceTags(src,"**","b")
		
		src=ReplaceTags(src,"*","em")
		
		src=ReplaceTags(src,"%","i")
		
		src=ReplaceTags2( src,"@","i")
		
		src=ReplaceEscs(src)
		
		If src.EndsWith("  ") src=src[..-2]+"<br>"
		
		Return src
	End
	
End
