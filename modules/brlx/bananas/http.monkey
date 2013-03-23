
Import brl.asyncevent

#If TARGET="html5"
Import "native/http.js"
#Endif

#If TARGET="html5"
Private
Extern
Function BBXMLHttpRequest:Void( req:String,url:String,result:String[] )
#Endif

public

Interface IOnGetPageComplete
	Method OnGetPageComplete:Void( page:String,source:IAsyncEventSource )
End

Class HttpRequest Implements IAsyncEventSource

	Method GetPage:Void( url:String,onComplete:IOnGetPageComplete )
		_onComplete=onComplete
		AddAsyncEventSource Self
		BBXMLHttpRequest "GET",url,_result
	End
	
	Method UpdateAsyncEvents:Void()
		If Not _result[0] Return
		Local page:=_result[1]
		RemoveAsyncEventSource Self
		_onComplete.OnGetPageComplete _result[1],Self
	End
	
	Private
	
	Field _onComplete:IOnGetPageComplete
	Field _result:String[2]

End
