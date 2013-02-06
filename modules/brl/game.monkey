
Import "native/game.${LANG}"

#BRL_GAME_IMPORTED=True

Extern

Class BBGame Extends Null

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
	Method PollJoystick:Bool( port:Int,joyx:Float[],joyy:Float[],joyz:Float[],buttons:Bool[] )
	Method OpenUrl:Void( url:String )
	Method SetMouseVisible( visible:Bool )

	Function Game:BBGame()

	'js
	'String PathToUrl( String path )
	'ArrayBuffer LoadData( String path )
	
	'as
	'String PathToUrl( String path )
	'ByteArray LoadData( String path )
	
	'cpp
	'FILE *OpenFile( String path,String mode )
	'unsigned char *LoadData( String path,int *length )
	
	'java
	'RandomAccessFile OpenFile( String path,String mode )
	'InputStream OpenInputStream( String path )
	'byte[] LoadData( String path )
	
	'cs
	'FileStream OpenFile( String path,FileMode mode )
	'Stream OpenInputStream( String path )
	'byte[] LoadData( String path )

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

Extern

#If TARGET="html5"

Class BBHtml5Game Extends BBGame
End

#Else If TARGET="flash"

Class BBFlashGame Extends BBGame
End

#Else If TARGET="android"

Class BBAndroidGame Extends BBGame
End

#Else If TARGET="glfw"

Class BBGlfwGame Extends BBGame
End

#Else If TARGET="xna"

Class BBXnaGame Extends BBGame
End

#Else If TARGET="psm"

Class BBPsmGame Extends BBGame
End

#Else If TARGET="ios"

Class BBIosGame Extends BBGame
End

#Else If TARGET="win8"

Class BBWin8Game Extends BBGame
End

#End
