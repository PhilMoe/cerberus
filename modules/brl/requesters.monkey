
#If TARGET<>"glfw" Or (HOST<>"winnt" And HOST<>"macos")
#Error "Requesters module unavailable on this target"
#Endif

Import "native/requesters.cpp"

Extern

Function Notify:Void( title:String,text:String,serious:Bool=False )="bbNotify"

Function Confirm:Bool( title:String,text:String,serious:Bool=False )="bbConfirm"

Function Proceed:Int( title:String,text:String,serious:Bool=false )="bbProceed"

Function RequestFile:String( title:String,extensions:String="",save:Bool=False,file:String="" )="bbRequestFile"

Function RequestDir:String( title:String,dir:String="" )="bbRequestDir"
