
Import dom

Import "websocket.js"

Extern

'Based on interfaces published at:
'
'http://www.w3.org/TR/websockets/
'http://www.whatwg.org/specs/web-apps/current-work/multipage/comms.html

Class MessageEvent Extends Event

	Field data$
	Field origin$
	Field lastEventId$
	
	'Field source:WindowProxy
	'Field ports:MessagePortArray

End

Class WebSocket Extends EventTarget

	Field URL$
	
	Field CONNECTING
	Field OPEN
	Field CLOSING
	Field CLOSED
	
	Field readyState
	Field bufferAmount
	
	Field protocol$
	
	Method send( data$ )
	Method close()
	
End

Function createWebSocket:WebSocket( url$ )="createWebSocket"
Function createWebSocket:WebSocket( url$,protocol$ )="createWebSocket2"
Function createWebSocket:WebSocket( url$,protocols$[] )="createWebSocket2"
