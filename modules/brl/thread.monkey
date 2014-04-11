
' Highly unsafe threads!!!!! Use at your own considerable risk!!!!!
'
' Monkey is not thread safe - nothing is safe to call.
'
' C++ threads must not create arrays or objects, or modify elements/variables/fields etc that contain arrays or objects. Strings might be OK.
'
' This is undocced, as it's mainly intended for use only by native code, but might be fun to play with in monkey...good for testing anyway!

#If Not BRL_THREAD_IMPLEMENTED
#If LANG="cpp" Or LANG="java" Or LANG="cs" Or LANG="js" Or LANG="as"
#BRL_THREAD_IMPLEMENTED=True
Import "native/thread.${LANG}"
#Endif
#Endif

#If Not BRL_THREAD_IMPLEMENTED
#Error "Native Thread class not implemented."
#Endif

Extern Private

Class BBThread

	Method Start:Void()
	Method IsRunning:Bool()
	Method Result:Object()
	
	'Call this inside Run__UNSAFE__ to duplicate any strings you need to pass to background threads.
	'Not pretty, but faster than atomically syncing String refcnt incs/decs so it'll do for now.
	'
#If LANG="cpp"
	Function Strdup:String( str:String )
#Endif

	Private

	Method Run__UNSAFE__:Void() Abstract
	
End

Public

Class Thread Extends BBThread

#If LANG<>"cpp"
	Function Strdup:String( str:String )
		Return str
	End
#Endif

End
