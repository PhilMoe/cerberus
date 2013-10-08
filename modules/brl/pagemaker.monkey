
Class PageMaker

	Method New( template:String )
		_template=template
		_scopes.Push _decls
	End
	
	Method Clear:Void()
		_decls.Clear
		_scopes.Clear
		_scopes.Push _decls
		_lists.Clear
	End
	
	Method SetString:Void( name:String,value:String )
		_scopes.Top().Set name,New StringObject( value )
	End
	
	Method BeginList:Void( name:String )
		Local list:=New Stack<StringMap<Object>>
		_scopes.Top().Set name,list
		_scopes.Push Null
		_lists.Push list
	End
	
	Method AddItem:Void()
		Local scope:=New StringMap<Object>
		_scopes.Pop
		_scopes.Push scope
		_lists.Top().Push scope
	End
	
	Method EndList:Void()
		_scopes.Pop()
		_lists.Pop
	End
	
	Method MakePage:String()

		_iters.Clear
		_lists.Clear
		_scopes.Clear
		_scopes.Push _decls
		
		Local output:=New StringStack

		Local i:=0,ifnest:=0,iftrue:=0
		
		Repeat
			Local i0:=_template.Find( "${",i )
			If i0=-1 Exit
			
			Local i1:=_template.Find( "}",i0+2 )
			If i1=-1 Exit
			
			Local cc:=(ifnest=iftrue)
			
			If cc And i<i0
				output.Push _template[i..i0]
			Endif
			
			i=i1+1
			
			Local bits:=_template[i0+2..i1].Split(" ").Resize(5)

			Select bits[0]
			Case "IF"
				ifnest+=1
				If cc
					Local i:=1,inv:=False
					
					If bits[i]="NOT" 
						inv=True
						i+=1
					Endif
					
					If bits[i]="FIRST"
						cc=(_iters.Get(_iters.Length-2)=0)
					Else If bits[i]="LAST"
						cc=(_iters.Get(_iters.Length-2)=_lists.Top().Length-1)
					Else If bits[i+1]="EQ"
						cc=GetString( bits[i] )=bits[i+2]
					Else If bits[i+1]="NE"
						cc=GetString( bits[i] )<>bits[i+2]
					Else
						cc=GetString( bits[i] )<>""
					Endif
					
					If inv cc=Not cc
					If cc iftrue=ifnest
					
				Endif
			Case "ENDIF"
				ifnest-=1
				iftrue=Min( iftrue,ifnest )
			Case "FOR"
				ifnest+=1
				If cc
					Local list:=GetList( bits[1] )
					If list
						iftrue=ifnest
						_iters.Push 0
						_iters.Push i
						_lists.Push list
						_scopes.Push list.Get( 0 )
					Endif
				Endif
			Case "NEXT"
				If cc
					_scopes.Pop
					Local list:=_lists.Top()
					Local p:=_iters.Pop()
					Local j:=_iters.Pop()+1
					If j<list.Length
						_iters.Push j
						_iters.Push p
						_scopes.Push list.Get( j )
						i=p
					Else
						_lists.Pop
					Endif
				Endif
				ifnest-=1
				iftrue=Min( iftrue,ifnest )
			Default
				If cc
					Local str:=GetString( bits[0] )
					If str output.Push str
				Endif
			End

		Forever
		
		If i<_template.Length output.Push _template[i..]

		Return output.Join( "" )
	End
	
	Private

	Field _template:String
	Field _decls:=New StringMap<Object>
	Field _scopes:=New Stack<StringMap<Object>>
	Field _lists:=New Stack<Stack<StringMap<Object>>>
	Field _iters:=New IntStack
	
	Method GetValue:Object( name:String )
		For Local i:=_scopes.Length-1 To 0 Step -1
			Local sc:=_scopes.Get( i )
			If sc.Contains( name ) Return sc.Get( name )
		Next
		Return Null
	End
	
	Method GetString:String( name:String )
		Local val:=GetValue( name )
		Local str:=StringObject( val )
		If str Return str.value
		Local list:=Stack<StringMap<Object>>( val )
		If list And list.Length Return String( list.Length )
		Return ""
	End
	
	Method GetList:Stack<StringMap<Object>>( name:String )
		Local val:=GetValue( name )
		Local list:=Stack<StringMap<Object>>( val )
		If list And list.Length Return list
		Return Null
	End
	
End


