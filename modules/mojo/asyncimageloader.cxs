
#If MOJO_VERSION_X
#Error "Mojo version error"
#Endif

Import mojo.graphics
Import mojo.asyncloaders

Import brl.asyncevent

Private

Import mojo.graphicsdevice
Import mojo.data
Import brl.thread

#If LANG="js" Or LANG="as"

Import "native/asyncimageloader.${LANG}"

Extern Private

Class AsyncImageLoaderThread="BBAsyncImageLoaderThread"

	Field _device:GraphicsDevice
	Field _path:String
	Field _surface:Surface
	Field _result:Bool
	
	Method Start:Void()
	Method IsRunning:Bool()
	
End

#Else

Class AsyncImageLoaderThread Extends Thread

	Field _device:GraphicsDevice
	Field _path:String
	Field _surface:Surface
	Field _result:Bool
	
	Method Start:Void()
		_surface=New Surface
#If TARGET="psm"	'PSM doesn't like loading textures on background thread?
		Run__UNSAFE__
#Else
		Super.Start
#Endif
	End
	
	Method Run__UNSAFE__:Void()
		_result=_device.LoadSurface__UNSAFE__( _surface,Strdup( _path ) )
	End

End

#Endif

Public

Class AsyncImageLoader Extends AsyncImageLoaderThread Implements IAsyncEventSource

	Method New( path:String,frames:Int=1,flags:Int=Image.DefaultFlags,onComplete:IOnLoadImageComplete )
		_device=GetGraphicsDevice()
		_mpath=path
		_path=FixDataPath( path )
		_frames=frames
		_flags=flags
		_onComplete=onComplete
	End

	Method Start:Void()
		AddAsyncEventSource Self
		Super.Start
	End
	
	Private
	
	Field _mpath:String
	Field _frames:Int
	Field _flags:Int
	Field _onComplete:IOnLoadImageComplete
	
	Method UpdateAsyncEvents:Void()
		If IsRunning() Return
		RemoveAsyncEventSource Self
		If _result 
			_surface.OnUnsafeLoadComplete()
			Local image:=(New Image).Init( _surface,_frames,_flags )
			_onComplete.OnLoadImageComplete image,_mpath,Self
		Else
			_onComplete.OnLoadImageComplete Null,_mpath,Self
		Endif
	End
	
End
