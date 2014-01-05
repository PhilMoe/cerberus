
#If MOJO_VERSION_X
#Error "Mojo version error"
#Endif

Import mojo.graphics
Import mojo.audio

Import brl.asyncevent

Private

Import mojo.asyncimageloader
Import mojo.asyncsoundloader

Public

Interface IOnLoadImageComplete
	Method OnLoadImageComplete:Void( image:Image,path:String,source:IAsyncEventSource )
End

Interface IOnLoadSoundComplete
	Method OnLoadSoundComplete:Void( sound:Sound,path:String,source:IAsyncEventSource )
End

Function LoadImageAsync:Void( path:String,frames:Int=1,flags:Int=Image.DefaultFlags,onComplete:IOnLoadImageComplete )
	Local loader:=New AsyncImageLoader( path,frames,flags,onComplete )
	loader.Start
End

Function LoadSoundAsync:Void( path:String,onComplete:IOnLoadSoundComplete )
	Local loader:=New AsyncSoundLoader( path,onComplete )
	loader.Start
End
