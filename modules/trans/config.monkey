
' Module trans.config
'
' Placed into the public domain 24/02/2011.
' No warranty implied; use at your own risk.

Import trans

Global ENV_HOST$
Global ENV_LANG$
Global ENV_CONFIG$
Global ENV_TARGET$
Global ENV_MODPATH$
Global ENV_SAFEMODE

Private

Class ConfigScope Extends ScopeDecl

	Field vars:=New StringMap<String>
	Field cdecls:=New StringMap<ConstDecl>
	
	Method FindValDecl:ValDecl( ident$ )
		If cdecls.Contains( ident ) Return cdecls.Get( ident )
		Return New ConstDecl( ident,DECL_SEMANTED,Type.boolType,Null )
	End

End

Global _cfgScope:=New ConfigScope
Global _cfgScopeStack:=New Stack<ConfigScope>

Public

Function PushConfigScope:Void()
	_cfgScopeStack.Push _cfgScope
	_cfgScope=New ConfigScope
End

Function PopConfigScope:Void()
	_cfgScope=_cfgScopeStack.Pop()
End

Function GetConfigScope:ScopeDecl()
	Return _cfgScope
End

Function SetConfigVar( key$,val$ )
	SetConfigVar( key,val,Type.stringType )
End

Function SetConfigVar( key$,val$,type:Type )
	Local decl:=_cfgScope.cdecls.Get( key )
	If decl
		decl.type=type
	Else
		decl=New ConstDecl( key,DECL_SEMANTED,type,Null )
		_cfgScope.cdecls.Set key,decl
	Endif
	decl.value=val
	If BoolType( type )
		If val val="1" Else val="0"
	Endif
	_cfgScope.vars.Set key,val
End

Function GetConfigVar$( key$ )
	Return _cfgScope.vars.Get( key )
End

Function GetConfigVarType:Type( key$ )
	Local decl:=_cfgScope.cdecls.Get( key )
	If decl Return decl.type
	Return Null
End

Function GetConfigVars:StringMap<String>()
	Return _cfgScope.vars
End

'Function RemoveConfigVar( key$ )
'	_cfgScope.cdecls.Remove key
'	_cfgScope.vars.Remove key
'End

Function EvalConfigTags$( cfg$ )
	Local i:=0
	Repeat
	
		i=cfg.Find( "${" )
		If i=-1 Return cfg

		Local e:=cfg.Find( "}",i+2 )
		If e=-1 Return cfg
		
		Local key:=cfg[i+2..e]
		Local val:=_cfgScope.vars.Get( key )
		
		cfg=cfg[..i]+val+cfg[e+1..]
		i+=val.Length
		
	Forever
End

Global _errInfo$
Global _errStack:=New StringList

Function PushErr( errInfo$ )
	_errStack.AddLast _errInfo
	_errInfo=errInfo
End

Function PopErr()
	_errInfo=_errStack.RemoveLast()
End

Function Err( err$ )
'	Print _errInfo+" : "+err
'	Error _errInfo+" : "+err
	Print _errInfo+" : Error : "+err
	ExitApp -1
End

Function InternalErr( err$="Internal error" )
	Print _errInfo+" : "+err
	Error _errInfo+" : "+err
End

Function IsSpace( ch )
	Return ch<=32
'	Return ch<=Asc(" ")
End

Function IsDigit( ch )
	Return ch>=48 And ch<=57
'	Return ch>=Asc("0") And ch<=Asc("9")
End

Function IsAlpha( ch )
	Return (ch>=65 And ch<=90) Or (ch>=97 And ch<=122)
'	Return (ch>=Asc("A") And ch<=Asc("Z")) Or (ch>=Asc("a") And ch<=Asc("z"))
End

Function IsBinDigit( ch )
	Return ch=48 Or ch=49
'	Return ch=Asc("0") Or ch=Asc("1")
End

Function IsHexDigit( ch )
	Return (ch>=48 And ch<=57) Or (ch>=65 And ch<=70) Or (ch>=97 And ch<=102)
'	Return IsDigit(ch) Or (ch>=Asc("A") And ch<=Asc("F")) Or (ch>=Asc("a") And ch<=Asc("f"))
End

Function Enquote$( str$,lang$ )
	Select lang
	Case "cpp","java","as","js","cs"
		str=str.Replace( "\","\\" )
		str=str.Replace( "~q","\~q" )
		str=str.Replace( "~n","\n" )
		str=str.Replace( "~r","\r" )
		str=str.Replace( "~t","\t" )
		For Local i=0 Until str.Length
			If str[i]>=32 And str[i]<128 Continue
			Local t$,n=str[i]
			While n
				Local c=(n&15)+48
				If c>=58 c+=97-58
				t=String.FromChar( c )+t
				n=(n Shr 4) & $0fffffff
			Wend
			If Not t t="0"
			Select lang
			Case "cpp"
				t="~q L~q\x"+t+"~q L~q"
			Default
				t="\u"+("0000"+t)[-4..]
			End
			str=str[..i]+t+str[i+1..]
			i+=t.Length-1
		Next
		Select lang
		Case "cpp"
			str="L~q"+str+"~q"
		Default
			str="~q"+str+"~q"
		End
		Return str
	End
	InternalErr
End

Function Dequote$( str$,lang$ )
	Select lang
	Case "monkey"
		If str.Length<2 Or Not str.StartsWith("~q") Or Not str.EndsWith("~q") InternalErr
		str=str[1..-1]
		Local i:=0
		Repeat
			i=str.Find( "~~",i )
			If i=-1 Exit
			If i+1>=str.Length Err "Invalid escape sequence in string"
			Local ch:=str[i+1..i+2]
			Select ch
			Case "~~" ch="~~"
			Case "q" ch="~q"
			Case "n" ch="~n"
			Case "r" ch="~r"
			Case "t" ch="~t"
			Case "u"
				Local t:=str[i+2..i+6]
				If t.Length<>4 Err "Invalid unicode hex value in string"
				For Local j:=0 Until 4
					If Not IsHexDigit( t[j] ) Err "Invalid unicode hex digit in string"
				Next
				str=str[..i]+String.FromChar( StringToInt( t,16 ) )+str[i+6..]
				i+=1
				Continue
			Case "0" ch=String.FromChar(0)	'"~0"
			Default
				Err "Invalid escape character in string: '"+ch+"'"
			End
			str=str[..i]+ch+str[i+2..]
			i+=ch.Length
		Forever
		Return str
	End
	InternalErr
End

Function IntToString:String( n:Int,base:Int=10 )
	If Not n Return "0"
	If n<0 Return "-"+IntToString( -n,base )
	Local t:String
	While n
		Local c=(n Mod base)+48
		If c>=58 c+=97-58
		t=String.FromChar( c )+t
		n=n/base
	Wend
	Return t
End

Function StringToInt:Int( str:String,base:Int=10 )
	Local i:=0
	Local l:=str.Length
	While i<l And str[i]<=32
		i+=1
	Wend
	Local neg:=False
	If i<l And (str[i]=43 Or str[i]=45)
		neg=(str[i]=45)
		i+=1
	Endif
	Local n:=0
	While i<l
		Local c:=str[i],t:Int
		If c>=48 And c<58
			t=c-48
		Else If c>=65 And c<=90
			t=c-55
		Else If c>=97 And c<=122
			t=c-87
		Else
			Exit
		Endif
		If t>=base Exit
		n=n*base+t
		i+=1
	Wend
	If neg n=-n
	Return n
End
