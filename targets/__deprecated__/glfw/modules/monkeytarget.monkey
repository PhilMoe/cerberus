
#BRL_GAMETARGET_IMPLEMENTED=True

#GLFW_VERSION=2

Import brl.gametarget

Import "native/wavloader.cpp"
Import "native/oggloader.cpp"

Import "native/glfwgame.cpp"
Import "native/monkeytarget.cpp"

Extern

Class GlfwVideoMode="BBGlfwVideoMode"
	Field Width:Int
	Field Height:Int
	Field RedBits:Int
	Field GreenBits:Int
	Field BlueBits:Int
End

Class GlfwGame Extends BBGame="BBGlfwGame"

	Function GetGlfwGame:GlfwGame()="GlfwGame"

	Method GetGlfwDesktopMode:GlfwVideoMode()
	Method GetGlfwVideoModes:GlfwVideoMode[]()
	Method SetGlfwWindow:Void( width:Int,height:Int,red:Int,green:Int,blue:Int,alpha:Int,depth:Int,stencil:Int,fullscreen:Bool )
	
End
