
Import brl.databuffer
Import brl.asyncevent

Private

Import thread

Public

Class AsyncDataLoader Extends Thread Implements IAsyncStreamSource

	Method Load:Void( path:String,onComplete:IOnLoadComplete )
		AddAsyncEventSource Self
		_buf=New DataBuffer
		_onComplete=onComplete
		_path=path
		Start
	Private
	
	Field _buf:DataBuffer
	Field _onComplete:IOnLoadComplete
	Field _path:String
	Field _result:Bool
	
	Method Run__UNSAFE__:Void()
		_result=_buf._Load( _path )
	End
	
	Method UpdateAsyncEvents:Void()
		If IsRunning() Return
		RemoveAsyncEventSource Self
		If Not _result
			_buf.Discard
			_buf=Null
		Endif
		_onComplete.OnLoadComplete _buf,Self
	End

End
