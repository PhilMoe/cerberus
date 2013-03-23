
Function IsSpace( ch )
	Return ch<=32
End

Function IsDigit:Bool( ch )
	Return ch>=48 And ch<=57
End

Function IsAlpha:Bool( ch )
	Return (ch>=65 And ch<=90) Or (ch>=97 And ch<=122)
End

Function IsIdent:Bool( ch )
	Return ch=95 Or IsAlpha( ch )
End

Function IsBinDigit:Bool( ch )
	Return ch=48 Or ch=49
End

Function IsHexDigit:Bool( ch )
	Return (ch>=48 And ch<=57) Or (ch>=65 And ch<=70) Or (ch>=97 And ch<=102)
End

Function Enquote:String( str:String )
End

Function Dequote:String( str:String )
End

Function HexToInt:Int( str:String)
End

Function IntToHex:String( dec:Int )
End

Function BinToInt:Int( str:String )
End

Function IntToBin:String( dec:Int )
End

Function EscapeCString:String( str:String )
End

Function UnescapeCString:String( str:String )
End

Function EscapeMonkeyString:String( str:String )
End

Function UnescapeMonkeyString:String( str:String )
End

Function ParseSpace:String( str:String,start:Int=0 )
	Local i:=start
	While i<str.Length And IsSpace( str[i] )
		i+=1
	Wend
	Return str[start..i]
End

Function ParseIdent:String( str:String,start:Int=0 )
	Local i:=start
	If i<str.Length And IsIdent( str[i] )
		i+=1
		While i<str.Length And IsAlpha( str[i] ) Or IsUnderscore( str[i] ) Or IsDigit( str[i] )
			i+=1
		Wend
	Wend
	Return str[start..i]
End

Function ParseHex:String( str:String,start:Int=0 )
	Local i:=start
	While i<str.Length And IsHexDigit( str[i] )
		i+=1
	Wend
	Return str[start..i]
End

Function ParseBin:String( str:String,start:Int=0 )
	Local i:=start
	While i<str.Length And IsBinDigit( str[i] )
		i+=1
	Wend
	Return str[start..i]
End

Function ParseNumber:String( str:String,start:Int=0 )
End
