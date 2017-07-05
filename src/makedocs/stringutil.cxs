
Function IsSpace:Bool( ch:Int )
	Return ch<=32
End

Function IsDigit:Bool( ch:Int )
	Return ch>=48 And ch<=57
End

Function IsAlpha:Bool( ch:Int )
	Return (ch>=65 And ch<=90) Or (ch>=97 And ch<=122)
End

Function IsBinDigit:Bool( ch:Int )
	Return ch=48 Or ch=49
End

Function IsHexDigit:Bool( ch:Int )
	Return (ch>=48 And ch<=57) Or (ch>=65 And ch<=70) Or (ch>=97 And ch<=102)
End

Function HtmlEscape:String( str:String )
	Return str.Replace( "&","&amp;" ).Replace( "<","&lt;" ).Replace( ">","&gt;" )
End
