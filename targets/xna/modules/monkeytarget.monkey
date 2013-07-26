
#BRL_GAMETARGET_IMPLEMENTED=True

Import brl.gametarget

Import "native/xnagame.cs"
Import "native/monkeytarget.cs"

Extern

Class XnaDisplayMode="BBXnaDisplayMode"
	Field Width:Int
	Field Height:Int
	Field Format:Int
End

Class XnaGame Extends BBGame="BBXnaGame"

	Function GetXnaGame:XnaGame()="XnaGame"
	
	Method GetXnaDesktopMode:XnaDisplayMode()
	Method GetXnaDisplayModes:XnaDisplayMode[]()
	Method SetXnaDisplayMode:Void( width:Int,height:Int,format:Int,fullscreen:Bool )
	
End

Public


