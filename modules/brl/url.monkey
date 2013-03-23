
Class Url

	Method New( url:String,scheme:String="",port:Int=0 )
		Local i:=url.Find( "://" )
		If i<>-1
			_scheme=url[..i]
			url=url[i+3..]
		Else
			_scheme=scheme
		Endif
		i=url.Find( "/" )
		If i<>-1
			_domain=url[..i]
			Local j:=_domain.Find( ":" )
			If j<>-1
				_port=Int( _domain[j+1..] )
				_domain=_domain[..j]
			Else
				_port=port
			Endif
			url=url[i+1..]
		Else
			_domain=url
			_port=port
			url=""
		Endif
		_path=url
	End
	
	Method ToString:String()
		Return _url
	End
	
	Method Scheme:String()
		Return _scheme
	End
	
	Method Domain:String()
		Return _domain
	End
	
	Method Port:Int()
		Return _port
	End
	
	Method Path:String()
		Return _path
	End
	
	Private
	
	Field _url:String
	Field _scheme:String
	Field _domain:String
	Field _port:Int
	Field _path:String
	
End

