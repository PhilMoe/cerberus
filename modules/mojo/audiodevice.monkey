
#If MOJO_VERSION_X
#Error "Mojo version error"
#Endif

Private

Import driver

Extern

Class AudioDevice="gxtkAudio"

	Method Suspend()
	Method Resume()
	
	Method LoadSample:Sample( path$ )
	Method PlaySample( sample:Sample,channel,flags )
	
	Method StopChannel( channel )
	Method PauseChannel( channel )
	Method ResumeChannel( channel )
	Method ChannelState( channel )
	Method SetVolume( channel,volume# )
	Method SetPan( channel,pan# )
	Method SetRate( channel,rate# )

	Method PlayMusic( path$,flags )
	Method StopMusic()
	Method PauseMusic()
	Method ResumeMusic()
	Method MusicState()
	Method SetMusicVolume( volume# )
	
	'INTERNAL - subject to change etc.
	Method LoadSample__UNSAFE__:bool( sample:Sample,path$ )
	
End

Class Sample="gxtkSample"

	Method Discard()
	
End
