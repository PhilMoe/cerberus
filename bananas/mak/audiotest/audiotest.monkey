
'A simple audio test app

Import mojo

Extern
Global UserAgent:String="navigator.userAgent"
Public

Class MyApp Extends App

	Field tkey,soundFmt$,musicFmt$
	Field shoot:Sound,shoot_chan,shoot_loop
	Field tinkle:Sound,tinkle_chan=4,tinkle_loop
	Field music$,music_on,music_paused
	
	Method OnCreate()
	
#If TARGET="glfw"
		'
		'GLFW supports WAV/OGG - OGG doesn't stream yet.
		'
		soundFmt="wav"
		musicFmt="ogg"
		'
#Elseif TARGET="html5"
		'
		'HTML5 supports WAV, OGG, MP3, M4A. However...
		'
		'IE wont play WAV/OGG
		'FF wont play MP3/M4A/WAV
		'
		soundFmt="wav"
		musicFmt="ogg"
		If UserAgent.Contains( "MSIE " ) Or UserAgent.Contains( "Trident/" ) Or UserAgent.Contains( "Edge/" )
			soundFmt="mp3"
			musicFmt="mp3"
		Endif
		'
#Elseif TARGET="flash"
		'
		'Flash supports MP3, M4A online, but only MP3 embedded.
		'
		soundFmt="mp3"
		musicFmt="mp3"
		'
#Elseif TARGET="android"
		'
		'Android supports WAV, OGG, MP3, M4A
		'
		soundFmt="wav"
		musicFmt="ogg"
		'
#Elseif TARGET="xna"
		'
		'XNA supports WAV, MP3, WMA
		'
		soundFmt="wav"
		musicFmt="wma"
		'
#Elseif TARGET="ios"
		'
		'iOS supports WAV, MP3, M4A, CAF, AIFF
		'
		soundFmt="wav"
		musicFmt="m4a"
		'
#Elseif TARGET="psm"
		'
		'PSS supports WAV for sounds, MP3 for music.
		'
		soundFmt="wav"
		musicFmt="mp3"
		'
#Elseif TARGET="winrt"
		'
		soundFmt="wav"
		musicFmt="wav"
		'
#End
		LoadStuff
				
		SetUpdateRate 15
	End
	
	Method LoadStuff()
		shoot=LoadSound( "shoot."+soundFmt )
		tinkle=LoadSound( "tinkle."+soundFmt )
		music="happy."+musicFmt
	End
	
	Method OnUpdate()
	
		If KeyHit( KEY_CLOSE ) Error ""
		
		Local tx#=TouchX(0)*(320.0/DeviceWidth)
		Local ty#=TouchY(0)*(480.0/DeviceHeight)
	
		Local key
		If TouchHit(0)
			Local y=ty/24
			If y>=0 And y<9 key=y+1
		Endif
	
		For Local i=KEY_1 To KEY_9
			If KeyHit(i)
				key=i-KEY_1+1
				Exit
			Endif
		Next
		
		If key tkey=key-1

		Select key
		Case 1
			PlaySound shoot,0
		Case 2
			PlaySound shoot,shoot_chan
			shoot_chan+=1
			If shoot_chan=3 shoot_chan=0
		Case 3
			PlaySound tinkle,3
		Case 4
			PlaySound tinkle,tinkle_chan
			tinkle_chan+=1
			If tinkle_chan=6 tinkle_chan=3
		Case 5
			If shoot_loop
				StopChannel 6
			Else
				PlaySound shoot,6,True
			Endif
			shoot_loop=Not shoot_loop
		Case 6
			If tinkle_loop
				StopChannel 7
			Else
				PlaySound tinkle,7,True
			Endif
			tinkle_loop=Not tinkle_loop
		Case 7
			If music_on
				StopMusic
				music_on=False
			Else
				PlayMusic music,1
				music_on=True
				music_paused=False
			Endif
		Case 8
			If music_on
				If music_paused
					ResumeMusic
					music_paused=False
				Else
					PauseMusic
					music_paused=True
				Endif
			Endif
		Case 9
			'shoot.Discard		'should work with/without...
			'tinkle.Discard
			LoadStuff
		End
	End
	
	Method OnRender()

		Scale DeviceWidth/320.0,DeviceHeight/480.0
		
		Cls
		
		SetColor 0,0,255
		DrawRect 0,tkey*24,320,24
		
		Translate 5,5
		
		SetColor 255,255,255		
		DrawText "1) Play 'shoot' through channel 0",0,0
		DrawText "2) Play 'shoot' through channel 0...2",0,24
		DrawText "3) Play 'tinkle' through channel 3",0,24*2
		DrawText "4) Play 'tinkle' through channel 3...5",0,24*3
		DrawText "5) Loop/Stop 'shoot' through channel 6",0,24*4
		DrawText "6) Loop/Stop 'tinkle' through channel 7",0,24*5
		DrawText "7) Loop/Stop music",0,24*6
		DrawText "8) Pause/Resume music",0,24*7
		DrawText "9) Reload sounds",0,24*8
		
		Local y=24*9
		For Local i=0 Until 8
			DrawText "ChannelState("+i+")="+ChannelState(i),0,y
			y+=24
		Next
		DrawText "MusicState="+MusicState(),0,y
		
	End
End

Function Main()

	New MyApp

End
