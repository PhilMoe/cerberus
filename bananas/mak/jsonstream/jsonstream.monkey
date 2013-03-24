
'Ok, little example of converting objects to/from JSON using reflection
'
'Also handles built in list, stack and map classes.
'
'CAN'T HANDLE CYCLES!!!!!

#REFLECTION_FILTER="jsonstream*|monkey.list|monkey.stack|monkey.map"

Import reflection

Class JsonStream

	Method New()
	End

	Method New( json$ )
		WriteJson json
	End
	
	Method New( obj:Object )
		WriteObject obj
	End
	
	Method ReadJson$()
		ValidateJson
		Local t:=json[pos..]
		peeked=""
		json=""
		pos=0
		Return t
	End

	Method ReadObject:Object()
	
		Local t:=Read()
		If Not t JsonErr
		
		If IsDigit( t[0] )
			If t.Contains( "e" ) Or t.Contains( "." ) Return BoxFloat( Float(t) )
			Return BoxInt( Int(t) )
		Else If t[0]="~q"[0]
			Return BoxString( t[1..-1] )
		Else If t="null"
			Return Null
		Endif
		
		If t<>"{" JsonErr
		Local name:=Read()
		If name.StartsWith( "~q" ) And name.EndsWith( "~q" ) name=name[1..-1]
		Local clas:=GetClass( name )
		If Not clas JsonErr
		If Read()<>":" JsonErr

		Local obj:Object
		
		If Peek()="["
		
			Read
			Local elems:=New Stack<Object>
			While Peek()<>"]"
				If Not elems.IsEmpty() And Read()<>"," JsonErr
				elems.Push ReadObject()
			Wend
			Read
	
			Local n:=elems.Length
			
			'get monkey base class
			Local mclas:=clas
			While mclas And Not mclas.Name.StartsWith( "monkey." )
				mclas=mclas.SuperClass
			Wend
			Local mname:=mclas.Name

			If mname.StartsWith( "monkey.list." ) Or mname.StartsWith( "monkey.stack." )
			
				Local elemty:=clas.GetMethod( "ToArray",[] ).ReturnType.ElementType
				Local add:=clas.GetMethod( "AddLast",[elemty] )
				If Not add add=clas.GetMethod( "Push",[elemty] )

				obj=clas.NewInstance()

				For Local i=0 Until n
					add.Invoke( obj,[elems.Get(i)] )
				Next
				
			Else If mname.StartsWith( "monkey.map." )
			
				Local objenumer:=clas.GetMethod( "ObjectEnumerator",[] )
				Local nextobj:=objenumer.ReturnType.GetMethod( "NextObject",[] )
				Local getkey:=nextobj.ReturnType.GetMethod( "Key",[] )
				Local getval:=nextobj.ReturnType.GetMethod( "Value",[] )
				Local set:=clas.GetMethod( "Set",[getkey.ReturnType,getval.ReturnType] )
				
				obj=clas.NewInstance()
				
				For Local i=0 Until n Step 2
					set.Invoke( obj,[elems.Get(i),elems.Get(i+1)] )
				Next
			
			Else
				obj=clas.NewArray( n )
				
				For Local i=0 Until n
					clas.SetElement( obj,i,elems.Get(i) )
				Next

			Endif
			
		Else If Peek()="{"
			Read
			obj=clas.NewInstance
			Local i:=0
			While Peek()<>"}"
				If i And Read()<>"," JsonErr
				Local id:=Read()
				If id.StartsWith("~q") And id.EndsWith("~q") id=id[1..-1]
				Local f:=clas.GetField( id )
				If Not f Error "Field '"+id+"' not found"
				If Read()<>":" JsonErr
				f.SetValue( obj,ReadObject() )
				i+=1
			Wend
			Read
		Else
			JsonErr
		Endif
		
		If Read()<>"}" JsonErr
		
		Return obj
	End
	
	Method WriteJson:Void( json$ )
		Write json
	End
	
	Method WriteObject:Void( obj:Object )

		If Not obj
			Write "null"
			Return
		Endif
	
		Local clas:=GetClass( obj )
		
		If clas=IntClass()
			Local t:=String( UnboxInt( obj ) ).ToLower()
			If t.Contains( "." ) t=t[..t.Find(".")]
			If t.Contains( "e" ) t=t[..t.Find("e")]
			Write t
			Return
		Else If clas=FloatClass()
			Local t:=String( UnboxFloat( obj ) ).ToLower()
			If Not t.Contains( "." ) And Not t.Contains( "e" ) t+=".0"
			Write t
			Return
		Else If clas=StringClass()
			Local t:=String( UnboxString( obj ) )
			t=t.Replace( "~q","~~q" )
			t="~q"+t+"~q"
			Write t
			Return
		Endif
		
		Local name:=clas.Name

		'write class..
		Write "{"
		Write "~q"+name+"~q"
		Write ":"
		
		'array?
		Local elemty:=clas.ElementType
		If elemty
			Write "["
			For Local i=0 Until clas.ArrayLength( obj )
				If i Write ","
				WriteObject clas.GetElement( obj,i )
			Next
			Write "]"
			Write "}"
			Return
		Endif
		
		'Get base monkey name/class
		Local mclas:=clas
		While mclas And Not mclas.Name.StartsWith( "monkey." )
			mclas=mclas.SuperClass
		Wend
		Local mname:=mclas.Name
		
		'list or stack?
		If mname.StartsWith( "monkey.list." ) Or mname.StartsWith( "monkey.stack." )
			Write "["
			Local toarr:=clas.GetMethod( "ToArray",[] )
			Local arrty:=toarr.ReturnType
			Local arr:=toarr.Invoke( obj,[] )
			Local len:=arrty.ArrayLength( arr )
			For Local i=0 Until len
				If i Write ","
				WriteObject arrty.GetElement( arr,i )
			Next
			Write "]"
		'map?
		Else If mname.StartsWith( "monkey.map." )
			Write "["
			Local objenumer:=clas.GetMethod( "ObjectEnumerator",[] )
			Local hasnext:=objenumer.ReturnType.GetMethod( "HasNext",[] )
			Local nextobj:=objenumer.ReturnType.GetMethod( "NextObject",[] )
			Local getkey:=nextobj.ReturnType.GetMethod( "Key",[] )
			Local getval:=nextobj.ReturnType.GetMethod( "Value",[] )
			Local enumer:=objenumer.Invoke( obj,[] ),n:=0
			While UnboxBool( hasnext.Invoke( enumer,[] ) )
				Local node:=nextobj.Invoke( enumer,[] )
				Local key:=getkey.Invoke( node,[] )
				Local val:=getval.Invoke( node,[] )
				If n Write ","
				WriteObject key
				Write ","
				WriteObject val
				n+=2
			Wend
			Write "]"
		'user object?
		Else
			'better not be cyclic..!
			Write "{"
			Local n:=0
			For Local f:=Eachin clas.GetFields( True )
				If n Write ","
				Write "~q"+f.Name+"~q"
				Write ":"
				WriteObject f.GetValue( obj )
				n+=1
			Next
			Write "}"
		Endif
		
		Write "}"
		
	End

	Private

	Field indent$
	Field json$,pos,peeked$
	Field buf:=New StringStack
	
	Method JsonErr()
		Error "Bad JSON!"
	End
	
	Method ValidateJson()
		If buf.Length
			json+=buf.Join("")
			buf.Clear
		Endif
	End
	
	Method IsDigit?( ch )
		Return ch>=48 And ch<=57
	End
	
	Method IsAlpha?( ch )
		Return (ch>=65 And ch<=90) Or (ch>=97 And ch<=122) Or (ch=95)
	End

	Method Peek$()
		If Not peeked peeked=Read()
		Return peeked
	End

	Method Read$()
		If peeked
			Local t:=peeked
			peeked=""
			Return t
		Endif

		ValidateJson
		
		While pos<json.Length And json[pos]<=32
			pos+=1
		Wend
		
		Local c:=Chr()
		If Not c Return
		
		Local st:=pos
		pos+=1
		
		If IsAlpha( c )
			While IsAlpha( Chr() ) Or IsDigit( Chr() )
				pos+=1
			Wend
		Else If IsDigit( c )
			SkipDigits
			If Chr()="."[0]
				pos+=1
				SkipDigits
			Endif
			If Chr()="e"[0]
				pos+=1
				If Chr()="+"[0] Or Chr()="-"[0] 
					pos+=1
				Endif
				SkipDigits
			Endif
		Else If c="~q"[0]
			While Chr() And Chr()<>"~q"[0]
				pos+=1
			Wend
			If Chr()="~q"[0] pos+=1
		Endif
		Return json[st..pos]
	End
	
	Method Write( t$ )
		Select t
		Case "{","["
			indent+=" "
			t+="~n"+indent
		Case "}","]"
			indent=indent[1..]
			t="~n"+indent+t
		Case ","
			t+="~n"+indent
		End
		buf.Push t
	End
	
	Private

	Method Chr()
		If pos<json.Length Return json[pos]
		Return 0
	End
	
	Method SkipDigits()
		While IsDigit( Chr() )
			pos+=1
		Wend
	End
	
End

'***** Demo of using stream *****

Class Vec3
	Field x#,y#,z#
	
	Method New( x#,y#,z# )
		Self.x=x
		Self.y=y
		Self.z=z
	End
End

Class Vec4 Extends Vec3
	Field w#

	Method New( x#,y#,z#,w# )
		Self.x=x
		Self.y=y
		Self.z=z
		Self.w=w
	End
End

Class Test
	Field x=100
	Field y#=200
	Field z$=300
	Field xs[]=[1,2]
	Field ys#[]=[1.0,2.0]
	Field zs$[]=["1","2"]
	Field intlist:=New IntList
	Field vecstack:=New Stack<Vec3>
	Field vecarray:=New Vec3[10]
	Field mapofvecs:=New StringMap<Vec3>
	
	Method New()
	End
	
	Method Init()
		intlist.AddLast 10
		intlist.AddLast 20
		intlist.AddLast 30
		vecstack.Push New Vec4(1,2,3,4)
		vecstack.Push Null
		vecstack.Push New Vec3(7,8,9)
		For Local i=0 Until 10 Step 2
			vecarray[i]=New Vec3( i,i*2,i*3 )
		Next
		mapofvecs.Set "A",New Vec3(1,2,3)
		mapofvecs.Set "B",New Vec4(4,5,6,7)
	End

End

Function Main()

	Local test:=New Test
	test.Init
	
	Local stream:=New JsonStream
	
	stream.WriteObject test
	Local json:=stream.ReadJson()

	stream.WriteJson json
	Local obj:=stream.ReadObject()

	stream.WriteObject obj
	Local json2:=stream.ReadJson()
	
	If json=json2
		Print json
		Print "SUCCESS!"
	Else
		Print json
		Print json2
		Print "ERROR!!!!!"
	Endif
	
End
