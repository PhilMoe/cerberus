
#If TARGET="stdcpp"

Function Main()
	Test
End

#Else

Import mojo

Class MyApp Extends App

	Method OnCreate()
		SetUpdateRate 60
	End
	
	Method OnUpdate()
		Test
	End
	
	Method OnRender()
	End
End

Function Main()
	New MyApp
End

#End

Class C
	Field x
End

Function Test()

	'Integer divide by zero
	Local x:=0,y:=0
'	Print x/y
	
	'Null object
	Local c:C
'	Print c.x

	Local p[0]
	Print p[1]

End

