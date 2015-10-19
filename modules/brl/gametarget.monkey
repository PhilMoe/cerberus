
#If LANG="cpp" Or LANG="java" Or LANG="cs" Or LANG="js" Or LANG="as"
Import "native/gametarget.${LANG}"
#Endif

Const BBWINDOW_FULLSCREEN:=1
Const BBWINDOW_RESIZABLE:=2
Const BBWINDOW_DECORATED:=4
Const BBWINDOW_FLOATING:=8
Const BBWINDOW_DEPTHBUFFER:=16
Const BBWINDOW_SINGLEBUFFER:=32
Const BBWINDOW_SECONDMONITOR:=64

Extern

Class BBDisplayMode
	Field width:Int
	Field height:Int
	Field depth:Int
	Field hertz:Int
	Field flags:Int
End

Class BBGame Extends Null

	Function Game:BBGame()

	Method SetDelegate:Void( delegate:BBGameDelegate )
	Method SetUpdateRate:Void( hertz:Int )
	Method SetKeyboardEnabled:Void( enabled:Bool )
	Method Millisecs:Int()
	Method GetDate:Void( date:Int[] )
	Method CurrentDate:String()
	Method CurrentTime:String()
	Method SaveState:Int( state:String )
	Method LoadState:String()
	Method LoadString:String( path:String )
	Method CountJoysticks:Int( update:Bool )
	Method PollJoystick:Bool( port:Int,joyx:Float[],joyy:Float[],joyz:Float[],buttons:Bool[] )
	Method OpenUrl:Void( url:String )
	Method SetMouseVisible( visible:Bool )
	
	Method GetDeviceWidth:Int()
	Method GetDeviceHeight:Int()
	Method SetDeviceWindow:Void( width:Int,height:Int,flags:Int )
	Method GetDisplayModes:BBDisplayMode[]()
	Method GetDesktopMode:BBDisplayMode()
	Method SetSwapInterval:Void( interval:Int )

	Method PathToFilePath:String( path:String )
End

Class BBGameDelegate Abstract
	Method StartGame:Void() Abstract
	Method SuspendGame:Void() Abstract
	Method ResumeGame:Void() Abstract
	Method UpdateGame:Void() Abstract
	Method RenderGame:Void() Abstract
	Method KeyEvent:Void( event:Int,data:Int ) Abstract
	Method MouseEvent:Void( event:Int,data:Int,x:Float,y:Float ) Abstract
	Method TouchEvent:Void( event:Int,data:Int,x:Float,y:Float ) Abstract
	Method MotionEvent:Void( event:Int,data:Int,x:Float,y:Float,z:Float ) Abstract
	Method DiscardGraphics:Void() Abstract
End

Public

Class BBGameEvent
	Const KeyDown:=1
	Const KeyUp:=2
	Const KeyChar:=3
	Const MouseDown:=4
	Const MouseUp:=5
	Const MouseMove:=6
	Const TouchDown:=7
	Const TouchUp:=8
	Const TouchMove:=9
	Const MotionMove:=10
End
