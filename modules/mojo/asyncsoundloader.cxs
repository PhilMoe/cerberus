
#If MOJO_VERSION_X
#Error "Mojo version error"
#Endif

Import mojo.audio
Import mojo.asyncloaders

Private

Import mojo.audiodevice
Import mojo.data
Import brl.thread

#If LANG="js" Or LANG="as"

Import "native/asyncsoundloader.${LANG}"

Extern Private

Class AsyncSoundLoaderThread="BBAsyncSoundLoaderThread"

	Field _device:AudioDevice
	Field _path:String
	Field _sample:Sample
	Field _result:Bool
	
	Method Start:Void()
	Method IsRunning:Bool()

End

#Else

Class AsyncSoundLoaderThread Extends Thread

	Field _device:AudioDevice
	Field _path:String
	Field _sample:Sample
	Field _result:Bool
	
	Method Start:Void()
		_sample=New Sample
#If TARGET="xna"	'XNA doesn't like loading sounds on background thread?
		Run__UNSAFE__
#Else		
		Super.Start
#Endif
	End
	
	Method Run__UNSAFE__:Void()
		_result=_device.LoadSample__UNSAFE__( _sample,Strdup( _path ) )
	End

End

#Endif

Public

Class AsyncSoundLoader Extends AsyncSoundLoaderThread Implements IAsyncEventSource

	Method New( path:String,onComplete:IOnLoadSoundComplete )
		_device=GetAudioDevice()
		_mpath=path
		_path=FixDataPath( path )
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
	Field _onComplete:IOnLoadSoundComplete
	
	Method UpdateAsyncEvents:Void()
		If IsRunning() Return
		RemoveAsyncEventSource Self
		If _result
			Local sound:=New Sound( _sample )
			_onComplete.OnLoadSoundComplete sound,_mpath,Self
		Else
			_onComplete.OnLoadSoundComplete Null,_mpath,Self
		Endif
	End
	
End
