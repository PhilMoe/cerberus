
Class jsondecoder
	Field txt$
	Field i
	Field curchr$
	Field things:List<jsonvalue>
	
	Method getnext(tokens$[],onlywhitespace=1)
		Local oldi=i
		While i<txt.Length
			Local c$=txt[i..i+1]
			i+=1
			For Local token$=Eachin tokens
				If c=token 
					curchr=c
					Return 1
				Endif
			Next
			If onlywhitespace And (Not (c=" " Or c="~t" Or c="~n" Or c="~r"))
				i-=1
				Return 0
			Endif
		Wend
		i=oldi
		Return 0
	End
	
	Method New(_txt$)
		txt=_txt
		things=New List<jsonvalue>
	End
	
	Method parse()
		While getnext(["{","["])
			Select curchr
			Case "{" 'new object
				Local o:jsonobject=parseobject()
				If Not o
					Print "error - couldn't parse object"
				Endif
				things.AddLast o
			Case "[" 'new array
				Local a:jsonarray=parsearray()
				If Not a
					Print "error - couldn't parse array"
				Endif
				things.AddLast a
			End Select
		Wend
	End	
	
	Method parseobject:jsonobject()
		Local o:jsonobject=New jsonobject
		While getnext(["~q","}"])
			Select curchr
			Case "~q"
				Local p:jsonpair=parsepair()
				If Not p
					Print "error reading pair"
				Endif
				o.pairs.AddLast p
				If Not getnext([",","}"])
					Print "error after reading pair - expected either , or }"
				Endif
				If curchr="}"
					Return o
				Endif
			Case "}"
				Return o
			End Select
		Wend
		
		Print "error reading Object - expected a } at least!"
	End
	
	Method parsepair:jsonpair()
		Local p:jsonpair=New jsonpair
		p.name=parsestring()
		If Not getnext([":"])
			Print "error reading pair - expected a :"
		Endif
		Local v:jsonvalue=parsevalue()
		If Not v
			Print "error reading pair - couldn't read a value"
		Endif
		p.value=v
		Return p
	End
	
	Method parsearray:jsonarray()
		Local a:jsonarray=New jsonarray
		While getnext(["~q","-","0","1","2","3","4","5","6","7","8","9","{","[","t","f","n","]"])
			Select curchr
			Case "~q","-","0","1","2","3","4","5","6","7","8","9","{","[","t","f","n"
				i-=1
				Local v:jsonvalue=parsevalue()
				a.values.AddLast v
				If Not getnext([",","]"])
					Print "error - expecting , or ]"
				Endif
				If curchr="]"
					Return a
				Endif
				
			Case "]"
				Return a
			End Select
		Wend
		Print "error - expecting a value or ]"
	End
	
	Method parsestring$()
		Local oldi=i
		Local s$=""
		
		While getnext(["~q","\"],0)
			s+=txt[oldi..i-1]
			Select curchr
			Case "~q"
				Return s
			Case "\"
				Select txt[i..i+1]
				Case "~q"
					s+="~q"
				Case "\"
					s+="\"
				Case "/"
					s+="/"
				Case "b"
					s+=String.FromChar(8)
				Case "f"
					s+=String.FromChar(12)
				Case "n"
					s+="~n"
				Case "r"
					s+="~r"
				Case "t"
					s+="~t"
				Case "u"
					s+=parseunicode()
				End Select
				i+=1
			End Select
			oldi=i
		Wend
	End
	
	Method parseunicode$()
		Local n=0
		For Local t=1 To 4
			n*=16
			Local c=txt[i+t]
			If c>48 And c<57
				n+=c-48
			Else If c>=65 And c<=70
				n+=c-55
			Else If c>=97 And c<=102
				n+=c-87
			Endif
		Next
		i+=4
		Return String.FromChar(n)
	End
	
	Method parsevalue:jsonvalue()
		If Not getnext(["~q","-","0","1","2","3","4","5","6","7","8","9","{","[","t","f","n"])
			Print "error - expecting the beginning of a value"
		Endif
		Local s$
		Select curchr
		Case "~q"
			s=parsestring()
			Return New jsonstringvalue(s,0)
		Case "-","0","1","2","3","4","5","6","7","8","9"
			Local n=parsenumber()
			Return New jsonnumbervalue(n)
		Case "{"
			Local o:jsonobject=parseobject()
			Return o
		Case "["
			Local a:jsonarray=parsearray()
			Return a
		Case "t"
			i+=3
			Return New jsonliteralvalue(1)
		Case "f"
			i+=4
			Return New jsonliteralvalue(0)
		Case "n"
			i+=2
			Return New jsonliteralvalue(-1)
		End Select
	End
	
	Method parsenumber()
		i-=1
		Local sign=1
		Local n=0
		Select txt[i..i+1]
		Case "-"
			i+=2
			Return parsenumber()*(-1)
		Case "0"
			i+=1
			If getnext(["."])
				n=parsefraction()
			Endif
		Case "1","2","3","4","5","6","7","8","9"
			n=parseinteger()
			If getnext(["."])
				n+=parsefraction()
			Endif
		End Select
		
		If txt[i..i+1]="e" Or txt[i..i+1]="E"
			i+=1
			Select String.FromChar(txt[i])
			Case "+"
				sign=1
			Case "-"
				sign=-1
			Default
				Print "error - not a + or - when reading exponent in number"
			End
			Local e:Int=parseinteger()
			n*=Pow(10,sign*e)
		Endif
		Return n
	End
			
	Method parsefraction()
		Local digits=0
		Local n=0
		While txt[i]>=48 And txt[i]<=57 And i<txt.Length
			n*=10
			n+=txt[i]-48
			i+=1
			digits+=1
		Wend
		n/=Pow(10,digits)
		If i=txt.Length
			Print "error - reached EOF while reading number"
		Endif
		Return n
	End
	
	Method parseinteger()
		Local n=0
		While txt[i]>=48 And txt[i]<=57 And i<txt.Length
			n*=10
			n+=txt[i]-48
			i+=1
		Wend
		If i=txt.Length
			Print "error - reached EOF while reading number"
		Endif
		Return n
	End
			
End Class

Class jsonvalue

	Method repr$(tabs$="")
		Return tabs
	End
End Class

Class jsonobject Extends jsonvalue
	Field pairs:List<jsonpair>

	Method New()
		pairs=New List<jsonpair>
	End
	
	Method addnewpair(txt$,value:jsonvalue)
		pairs.AddLast (New jsonpair(txt,value))
	End
	
	Method repr$(tabs$="")
		Local t$="{"
		Local ntabs$=tabs+"~t"
		Local op:jsonpair=Null
		For Local p:jsonpair=Eachin pairs
			If op Then t+=","
			t+="~n"+ntabs+p.repr(ntabs)
			op=p
		Next
		t+="~n"+tabs+"}"
		Return t
	End
	
	Method getvalue:jsonvalue(name$)
		For Local p:jsonpair=Eachin pairs
			If p.name=name
				Return p.value
			Endif
		Next
	End
	
	Method getstringvalue$(name$)
		Local v:jsonstringvalue=jsonstringvalue(getvalue(name))
		If v
			Return v.txt
		Endif
	End
	
	Method getnumbervalue(name$)
		Local v:jsonnumbervalue=jsonnumbervalue(getvalue(name))
		If v
			Return v.number
		Endif
	End
	
	Method getliteralvalue(name$)
		Local v:jsonliteralvalue=jsonliteralvalue(getvalue(name))
		If v
			Return v.value
		Endif
	End
	
	Method getarrayvalue:jsonarray(name$)
		Local v:jsonarray=jsonarray(getvalue(name))
		Return v
	End
	
	Method getobjectvalue:jsonobject(name$)
		Local v:jsonobject=jsonobject(getvalue(name))
		Return v
	End
				
	
End Class

Class jsonpair
	Field name$,value:jsonvalue

	Method New(_name$,_value:jsonvalue)
		name=_name
		value=_value
	End

	Method repr$(tabs$="")
		Local t$="~q"+name+"~q : "
		For Local i=1 To (t.Length+7)/8
			tabs+="~t"
		Next
		Local middo$=""
		For Local i=1 To (8-(t.Length Mod 8))
			middo+=" "
		Next
		Return t+middo+value.repr(tabs)
	End
End Class

Class jsonarray Extends jsonvalue
	Field values:List<jsonvalue>
	
	Method New()
		values=New List<jsonvalue>
	End
	
	Method repr$(tabs$="")
		Local t$="["
		Local ntabs$=tabs+"~t"
		Local ov:jsonvalue=Null
		For Local v:jsonvalue=Eachin values
			If ov Then t+=","
			t+="~n"+ntabs+v.repr(ntabs)
			ov=v
		Next
		t+="~n"+tabs+"]"
		Return t
	End
End Class


Class jsonstringvalue Extends jsonvalue
	Field txt$
	
	Method New(_txt$,pretty=1)
		If pretty
			Local otxt$=""
			Local i=0
			For i=0 To _txt.Length-1
				Select _txt[i..i+1]
				Case "~q"
					otxt+="\~q"
				Case "\"
					otxt+="\\"
				Case "/"
					otxt+="\/"
				Case String.FromChar(8)
					otxt+="\b"
				Case String.FromChar(12)
					otxt+="\f"
				Case "~n"
					otxt+="\n"
				Case "~r"
					otxt+="\r"
				Case "~t"
					otxt+="\t"
				Default
					otxt+=txt[i..i+1]
				End Select
			Next
			txt=otxt
		Else
			txt=_txt
		Endif
	End
	
	Method repr$(tabs$="")
		Return "~q"+txt+"~q"
	End
End Class

Class jsonnumbervalue Extends jsonvalue
	Field number
	
	Method New(n)
		number=n
	End
	
	Method repr$(tabs$="")
		Return String(number)
	End
End Class

Class jsonliteralvalue Extends jsonvalue
	Field value
	'1 - true
	'0 - false
	'-1 - nil
	
	Method New(_value)
		value=_value
	End
	
	Method repr$(tabs$="")
		Select value
		Case 1
			Return "true"
		Case 0
			Return "false"
		Case -1
			Return "nil"
		End Select
	End
End Class

Function Main()
	Print "have a look at the javascript console to see proper pretty printing"
	
	'EXAMPLE
	Local txt$="{~qthis~q: [1,2,0.1], ~qthat~q: ~qA long string with a ~nnewline~q}"
	Print txt
	
	Local j:jsondecoder=New jsondecoder(txt)

	j.parse()
	
	For Local v:jsonvalue=Eachin j.things
		Print v.repr()
	Next
	
End