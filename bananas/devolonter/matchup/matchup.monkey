
#Rem
	Code by Arthur "devolonter" Bikmullin
	Graphics by Olga "AhNinniah" Bikmullina
#End

Import src.game

#If TARGET = "android" Then
	#ANDROID_SCREEN_ORIENTATION="landscape"
#End

Function Main()
	New Game()
End Function
