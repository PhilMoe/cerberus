
Import mojo

Class MyApp Extends App

	Field str$="Started!",old$
	Field enabled?

	Method OnCreate()
		SetUpdateRate 15
	End
	
	Method OnUpdate()
	
		If KeyHit( KEY_MENU )
			Print "Menu key hit!"
		Else If KeyHit( KEY_SEARCH )
			Print "Search key hit!"
		Endif
		
		If enabled
			Repeat
				Local char=GetChar()
				If Not char Exit
				
				Print "char="+char
				If char>=32
					str+=String.FromChar( char )
				Else
					Select char
					Case 8
						str=str[..-1]
					Case 13
						If str.Trim()="bye" Error ""	'test abrupt exit with keyboard open
						enabled=False
						DisableKeyboard
					Case 27
						str=old
						enabled=False
						DisableKeyboard
					End
				Endif

			Forever
		Else
			If KeyHit( KEY_LMB )
				old=str
				str=""
				enabled=True
				EnableKeyboard
			Endif
		Endif
	End
	
	Method OnRender()
		Cls
		Scale 2,2
		DrawText "Text: "+str,0,0
		If Not enabled DrawText "Click anywhere to type text...",0,20
	End

End

Function Main()
	New MyApp
End

