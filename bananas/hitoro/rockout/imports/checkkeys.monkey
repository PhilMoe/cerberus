
Import rockout

Function CheckKeys ()

	' Player input (except mouse input, used universally)...
	
	If KeyHit (KEY_ESCAPE)
		GameSession.SetState STATE_MENU
	Endif
	
	If KeyHit (KEY_P)
		GameSession.SetState STATE_PAUSED
	Endif

	If KeyHit (KEY_SPACE)
		New Block (DEFAULT_BLOCK, Rnd (VDeviceWidth), Rnd (VDeviceHeight), 0, 0, 0.2, 0.2)
	Endif

	If KeyHit (KEY_BACKSPACE)
		GameSession.Player.Damage 100
	Endif
	
	If KeyDown (KEY_LEFT)
		AdjustVirtualZoom -0.01
	Endif

	If KeyDown (KEY_RIGHT)
		AdjustVirtualZoom 0.01
	Endif
	
	If KeyHit (KEY_ENTER)
		SetVirtualZoom 1.0
	Endif
	
	If KeyHit (KEY_UP)
	
		UPDATE_RATE = UPDATE_RATE + 5
		SetUpdateRate UPDATE_RATE
		
	Endif
	
	If KeyHit (KEY_DOWN)
	
		UPDATE_RATE = UPDATE_RATE - 5
		If UPDATE_RATE < 5 Then UPDATE_RATE = 5
		
		SetUpdateRate UPDATE_RATE
		
	Endif
	
End
