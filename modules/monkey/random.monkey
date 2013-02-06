
' Module monkey.random
'
' Placed into the public domain 24/02/2011.
' No warranty implied; use at your own risk.

Private

Const A=1664525
Const C=1013904223

Public

Global Seed=1234

'Note: Freaky '|0' below is a work around for Js not having a real int type.
'
'Should probably be & $ffffffff? Think about it later!
'
Function Rnd#()
	Seed=(Seed*A+C)|0
	Return Float(Seed Shr 8 & $ffffff)/$1000000
End

Function Rnd#( range# )
	Return Rnd()*range
End Function

Function Rnd#( low#,high# )
 	Return Rnd( high-low )+low
End
