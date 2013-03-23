
Import os

Class Page

	Method New( template:String )
		_template=template
		_scopes.Push New StringMap<Object>
	End
	
	Method Clear()
		_scopes.Clear
		_scopes.Push New StringMap<Object>
		_lists.Clear
	End
	
	Method Set:Void( name:String,value:String )
		SetValue name,New StringObject(value)
	End
	
	Method BeginList:Void( name:String )
		Local list:=New Stack<StringMap<Object>>
		SetValue name,list
		_lists.Push list
	End
	
	Method AddItem:Void()
		Local list:=_lists.Top()
		If Not list Error "AddItem error"
		list.Push New StringMap<Object>
	End
	
	Method EndList:Void()
		If Not _lists.Length Error "EndList error"
		_lists.Pop
	End
	
	Method GeneratePage:String()

		If _lists.Length<>0 Error "MakePage error"
		If _scopes.Length<>1 Error "MakePage error"
		
		Local ifnest:=0,iftrue:=0
		Local i:=0,output:=New StringStack
		Local iter:ListIter,iters:=New Stack<ListIter>
		
		Repeat
			Local i0:=_template.Find( "${",i )
			If i0=-1 Exit
			
			Local i1:=_template.Find( "}",i0+2 )
			If i1=-1 Exit
			
			Local cc:=(ifnest=iftrue)
			
			If cc And i<i0 output.Push _template[i..i0]
			
			i=i1+1;
			
			Local bits:=_template[i0+2..i1].Split(" ").Resize(5)

			Select bits[0]
			Case "IF"
				ifnest+=1
				If cc
					Local i:=1
					If bits[1]="NOT" i=2
					If bits[i]="FIRST"
						cc=(iter.index=0)
					Else If bits[i]="LAST"
						cc=(iter.index=iter.list.Length-1)
					Else If bits[i+1]="EQ"
						cc=GetString(bits[i])=bits[i+2]
					Else If bits[i+1]="NE"
						cc=GetString(bits[i])<>bits[i+2]
					Else
						cc=GetString(bits[i])<>""
					Endif
					If i=2 cc=Not cc
					If cc iftrue=ifnest
				Endif
			Case "ENDIF"
				ifnest-=1
				iftrue=Min(iftrue,ifnest)
			Case "FOR"
				ifnest+=1
				If cc
					Local list:=GetList(bits[1])
					If list
						iftrue=ifnest
						iters.Push iter
						iter=New ListIter(list,i)
						_scopes.Push list.Get(0)
					Endif
				Endif
			Case "NEXT"
				If cc
					_scopes.Pop
					iter.index+=1
					If iter.index<iter.list.Length
						_scopes.Push iter.list.Get(iter.index)
						i=iter.loopi
					Else
						iter=iters.Pop()
					Endif
				Endif
				ifnest-=1
				iftrue=Min(iftrue,ifnest)
			Default
				If cc
					Local val:=GetString(bits[0])
					If val output.Push val
				Endif
			End

		Forever
		
		If i<_template.Length output.Push _template[i..]

		Return output.Join("")
	End

	Private	

	Field _template:String
	Field _scopes:=New Stack<StringMap<Object>>
	Field _lists:=New Stack<Stack<StringMap<Object>>>
	
	Method SetValue:Void( key:String,value:Object )
		If _lists.Length
			Local list:=_lists.Top()
			If Not list.Length Error "SetValue error"
			list.Top().Set key,value
			Return
		Endif
		_scopes.Top().Set key,value
	End
	
	Method GetValue:Object( key:String )
		For Local i:=_scopes.Length-1 To 0 Step -1
			Local sc:=_scopes.Get(i)
			If sc And sc.Contains(key) Return sc.Get(key)
		Next
		Return Null
	End
	
	Method GetList:Stack<StringMap<Object>>( key:String )
		Local value:Object=GetValue(key)
		Local list:=Stack<StringMap<Object>>(value)
		If list And list.Length
			Return list
		Endif
		Return Null
	End
	
	Method GetString:String( key:String )
		Local value:Object=GetValue(key)
		Local strobj:=StringObject(value)
		If strobj
			Return strobj.value
		Endif
		Local list:=Stack<StringMap<Object>>(value)
		If list And list.Length
			Return String(list.Length)
		Endif
		Return ""
	End
	
End

Class ListIter
	Field list:Stack<StringMap<Object>>
	Field loopi:Int
	Field index:Int
	
	Method New( list:Stack<StringMap<Object>>,loopi:Int )
		Self.list=list
		Self.loopi=loopi
		Self.index=0
	End 
End

