
Extern

'OK, minimal abstract app class for use by Mojo/GL apps etc.
'
'Load/save string/state moved elsewhere...
'
'App creation is target dependent.
'
Class BBGame
	Method SetDelegate:Void( delegate:BBGameDelegate )
	Method SetUpdateRate:Void( hertz:Int )
	Method SetKeyboardEnabled:Void( enabled:Bool )
	Method Width:Int()
	Method Height:Int()
	Method AccelX:Float()
	Method AccelY:Float()
	Method AccelZ:Float()
	Method Millisecs:Int()
End

Interface BBGameDelegate
	Method StartGame:Void()
	Method SuspendGame:Void()
	Method ResumeGame:Void()
	Method ResizeGame:Void( width:Int,height:Int )
	Method UpdateGame:Void()
	Method RenderGame:Void()
	Method KeyEvent( event:Int,key:Int )
	Method MouseEvent( event:Int,button:Int,x:Float,y:Float )
	Method TouchEvent( event:Int,finger:Int,x:Float,y:Float )
	Method DiscardGraphics:Void()
End

Public

Class BBEvent
	Const KeyDown:=1
	Const KeyUp:=2
	Const KeyChar:=3
	Const MouseDown:=4
	Const MouseUp:=5
	Const MouseMove:=6
	Const TouchDown:=7
	Const TouchUp:=8
	Const TouchMove:=9
End

