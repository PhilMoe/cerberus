
' Highly unsafe threads!!!!! Use at your own considerable risk!!!!!
'
' Monkey is not thread safe - nothing is safe to call.
'
' C++ threads must not create arrays or objects, or modify elements/variables/fields etc that contain arrays or objects. Strings might be OK.
'
' This is undocced, as it's mainly intended for use only by native code, but might be fun to play with in monkey...good for testing anyway!

#If LANG="cpp" Or LANG="java" Or LANG="cs" Or LANG="js" Or LANG="as"
#BRL_THREAD_IMPLEMENTED=True
Import "native/thread.${LANG}"
#Endif

#BRL_THREAD_IMPLEMENTED=False
#If BRL_THREAD_IMPLEMENTED="0"
#Error "Native Thread class not found."
#Endif

Extern

Class BBThread

	Method Start:Void()
	Method IsRunning:Bool()

	Private

	Method Run__UNSAFE__:Void() Abstract
	
End

Public

#If LANG="cpp"

Class Thread Extends BBThread

	Method New()
		'flush zombies!
		For Local thread:=Eachin _zombies
			If thread.IsRunning() _zombies2.Push thread
		Next
		_zombies=_zombies2
		_zombies2.Clear
		_alive.Push Self
	End
	
	Method Discard:Void()
		If IsRunning() _zombies.Push Self
		_alive.RemoveEach Self
	End
	
	Private
	
	'These keep threads alive to prevent GC prematurely reclaiming them...
	Global _alive:=New Stack<Thread>
	Global _zombies:=New Stack<Thread>
	Global _zombies2:=New Stack<Thread>

End

#Else If LANG="java" Or LANG="cs"

Class Thread Extends BBThread

	Method Discard:Void()
	End
	
End

#Else

Class Thread Extends BBThread

	Method Discard:Void()
	End
End

#Endif
