
' Highly unsafe threads!!!!! Use at your own considerable risk!!!!!
'
' Monkey is not thread safe - nothing is safe to call.
'
' C++ threads must not create or modify arrays or objects, or modify variables/fields etc that contain arrays or objects. Strings might be OK.
'
' This is undocced, as it's mainly intended for use only by native code, but might be fun to play with in monkey...good for testing anyway!
'
#If LANG="cpp" Or LANG="java"

#If LANG="cpp"
Import "native/thread.cpp"
#Elseif LANG="java"
Import "native/thread.java"
#Endif

Extern

Class BBThread

	Method Start:Void()
	Method IsRunning:Bool()
	Method Wait:Void()
	
	Private
	
	Method Run:Void() Abstract
	
End

Public

Class Thread Extends BBThread

End

#Endif
