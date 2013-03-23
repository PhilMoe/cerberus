
Private

Global _env:=New StringMap
Global _eval:=New StringMap

Public

Function SetEnv:Void( str:String )
	For Local line:=Eachin str.Split("~n")
		Local i:=line.Find("=")
		If i=-1 Continue
		Local lhs:=line[..i].Trim()
		Local rhs:=line[i+1..].Trim()
		SetEnv lhs,rhs
	Next
End

Function SetEnv:Bool( key:String,value:String )
	If _eval.Contains( key ) Return False
	_env.Set key,value
	Return True
End

Function GetEnv:String( key:String )
	Return _env.Get( key )
End

Function EvalEnv:String( key:String )
	If _eval.Contains( key ) Return _eval.Get( key )
	Local eval:=EvalEnvString( _env.Get( key ) )
	_eval.Set key,eval
	Return eval
End

Function EvalEnvString:String( str:String )
	Local i:=0
	Repeat
		Local i0:=str.Find( "${",i )
		If i0=-1 Return str
		Local i1:=str.Find( "}",i0+2 )
		If i1=-1 Return str
		
		i=i0+2
		
		If str[i0-2..i]="//" Continue
		
		Local key:=str[i0+2..i1]
		If Not _env.Contains( key ) Continue
		
		str=str[..i0]+eval+str[i1+1..]
		i=i0+eval.Length
	Forever
End