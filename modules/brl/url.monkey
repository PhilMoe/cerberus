
Class Url
	Method New(url:String, scheme:String = "", port:Int = 0)
		Set(url)
		
		'now override with passed in variables
		If scheme.Length _scheme = scheme
		If port > 0 _port = port
	End
	
	Method Set:Void(url:String)
		_url = url
		_scheme = "http"
		_user = ""
		_pass = ""
		_domain = ""
		_port = 80
		_path = "/"
		_query = ""
		_anchor = ""
		
		'start parsing url
		Local pos1:Int
		Local pos2:Int
		Local cursor:Int = 0
		
		'find _query and _anchor
		Local queryPos:= url.Find("?")
		Local anchorPos:= url.Find("#")
		
		'find non data length
		Local nonDataLength:Int
		If queryPos = -1 And anchorPos = -1
			'has no _query or _anchor
			nonDataLength = url.Length
		ElseIf queryPos > - 1 And anchorPos > - 1
			If queryPos < anchorPos
				'has _query and _anchor
				nonDataLength = queryPos
			Else
				'has just _anchor
				nonDataLength = anchorPos
				queryPos = -1
			EndIf
		ElseIf queryPos > - 1
			'has _query
			nonDataLength = queryPos
		Else
			'has just _anchor
			nonDataLength = anchorPos
		EndIf
		
		'find _scheme
		pos1 = url.Find("://", cursor)
		If pos1 > - 1 And pos1 < nonDataLength
			_scheme = url[cursor .. pos1]
			'move cursor
			cursor = pos1 + 3
		EndIf
		
		'find _user/_pass
		pos1 = url.Find("@", cursor)
		If pos1 > - 1 And pos1 < nonDataLength
			'find split
			pos2 = url.Find(":", cursor)
			If pos2 > - 1 And pos2 < pos1
				'_user and _pass
				_user = url[cursor .. pos2]
				_pass = url[pos2 + 1 .. pos1]
			Else
				'just _user
				_user = url[cursor .. pos1]
			EndIf
			
			'move cursor
			cursor = pos1 + 1
		EndIf
		
		'find _path and _port so we can figure out the address part
		Local portStart:= url.Find(":", cursor)
		Local pathStart:= url.Find("/", cursor)
		Local serverLength:Int
		
		'fix _port/_path start to be within non data section of url
		If portStart > - 1 And portStart >= nonDataLength portStart = -1
		If pathStart > - 1 And pathStart >= nonDataLength pathStart = -1
		
		If portStart = -1 And pathStart = -1
			'has no _port or _path
			_domain = url[cursor .. nonDataLength]
		ElseIf portStart > - 1 And pathStart > - 1
			If portStart < pathStart
				'has _port and _path
				_domain = url[cursor .. portStart]
				_port = Int(url[portStart + 1 .. pathStart])
				_path = url[pathStart .. nonDataLength]
			Else
				'has just _path
				_domain = url[cursor .. pathStart]
				_path = url[pathStart .. nonDataLength]
			EndIf
		ElseIf portStart > - 1
			'has just _port
			_domain = url[cursor .. portStart]
			_port = Int(url[portStart + 1 .. nonDataLength])
		Else
			'has just _path
			_domain = url[cursor .. pathStart]
			_path = url[pathStart .. nonDataLength]
		EndIf
		
		'find _query
		If queryPos > - 1
			If anchorPos > - 1
				'_query up until _anchor
				_query = url[queryPos + 1 .. anchorPos]
			Else
				'just up until end
				_query = url[queryPos + 1 ..]
			EndIf
		EndIf
		
		'find _anchor
		If anchorPos > - 1 _anchor = url[anchorPos + 1 ..]
	End
	
	Method ToString:String()
		Return _url
	End
	
	Method DebugString:String(seperator:String = "~n")
		Return "url: " + _url + seperator + "scheme: " + _scheme + seperator + "user: " + _user + seperator + "pass: " + _pass + seperator + "domain: " + _domain + seperator + "port: " + _port + seperator + "path: " + _path + seperator + "query: " + _query + seperator + "anchor: " + _anchor
	End
	
	Method Scheme:String()
		Return _scheme
	End
	
	Method User:String()
		Return _user
	End
	
	Method Pass:String()
		Return _pass
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
	
	Method Query:String()
		Return _query
	End
	
	Method Anchor:String()
		Return _anchor
	End
	
	Private
	
	Field _url:String
	Field _scheme:String
	Field _user:String
	Field _pass:String
	Field _domain:String
	Field _port:Int
	Field _path:String
	Field _query:String
	Field _anchor:String
End

#rem
//test cases
Function Main:Int()
	Local url:= New Url("?query=123")
	Print url.DebugString(", ")
	
	url.Set("?query=123#anchor")
	Print url.DebugString(", ")
	
	url.Set("monkey://?query=123#anchor")
	Print url.DebugString(", ")
	
	url.Set("monkey://:81234?query=123#anchor")
	Print url.DebugString(", ")

	url.Set("monkey://user@?query=123#anchor")
	Print url.DebugString(", ")
		
	url.Set("monkey://user:pass@?query=123#anchor")
	Print url.DebugString(", ")
	
	url.Set("monkey://user:pass@domain.com?query=123#anchor")
	Print url.DebugString(", ")
	
	url.Set("http://user:pass@domain.com/pat/goes/here?query=123#anchor")
	Print url.DebugString(", ")
	
	url.Set("http://user:pass@domain.com#/pat/goes/here?query=123#anchor")
	Print url.DebugString(", ")
End
#end
