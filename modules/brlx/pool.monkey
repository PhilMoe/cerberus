
Class Pool<T>

	Function AllocTmp:T()
		_tmp+=1
		If _tmp=_tmps.Length
			_tmps=_tmps.Resize( _tmps.Length*2+10 )
			For Local i=_tmp Until _tmps.Length
				_tmps[i]=New T
			Next
		Endif
		Return _tmps[_tmp]
	End
	
	Function PushTmps:Void()
		_stack.Push _tmp
	End
	
	Function PopTmps:Void()
		_tmp=_stack.Pop()
	End
	
	Private

	Global _tmp=-1
	Global _tmps:T[]
	Global _stack:=New IntStack
	
End
