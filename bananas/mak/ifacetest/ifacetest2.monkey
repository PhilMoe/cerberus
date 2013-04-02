
Import mojo

Interface I
	Method Yo()
End

Interface J
	Method Ha()
End

Class C
End

Class D Extends C Implements I
	Method Yo()
		Print "Yo."
	End
End

Function Main()

	Local c:=New C
	
	If I(c) Print "Fail"

	Local d:=New D
	
	If Not I(d) Print "Fail"
	
	Local o:Object
	
	o=c
	If I(o) Print "Fail"

	o=d
	If Not I(o) Print "Fail"
	
	Local i:I
	
	i=d
	If J(i) Print "Fail"
	If Not C(i) Print "Fail"
	If Not D(i) Print "Fail"
	
	Print "Bye!"
End
