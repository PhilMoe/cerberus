
Private

Global _sources:=New Stack<IAsyncEventSource>
Global _current:IAsyncEventSource	'for handling removal of current item.

Public

Interface IAsyncEventSource
	Method UpdateAsyncEvents:Void()
End

Function AddAsyncEventSource:Void( source:IAsyncEventSource )
	If _sources.Contains( source ) Error "Async event source is already active"
	_sources.Push source
End

Function RemoveAsyncEventSource:Void( source:IAsyncEventSource )
	If source=_current _current=Null
	_sources.RemoveEach source
End

Function UpdateAsyncEvents()
	If _current Return
	Local i:=0
	While i<_sources.Length
		_current=_sources.Get(i)
		_current.UpdateAsyncEvents
		If _current i+=1
	Wend
	_current=Null
End
