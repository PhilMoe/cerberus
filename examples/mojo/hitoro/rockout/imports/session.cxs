
Import rockout

' -----------------------------------------------------------------------------
' Constants used to set/track game state...
' -----------------------------------------------------------------------------

Const STATE_MENU		= 1
Const STATE_PLAYING	= 2
Const STATE_PAUSED	= 3
Const STATE_LOADLEVEL	= 4
Const STATE_GAMEOVER	= 5
Const STATE_STARTGAME	= 6

' -----------------------------------------------------------------------------
' Current game session...
' -----------------------------------------------------------------------------

Global GameSession:Session

' -----------------------------------------------------------------------------
' Session contains general gameplay session information...
' -----------------------------------------------------------------------------

Class Session

	' Ints and floats -- reset before new game!
	
	Global GameState:Int
	Global Score:Int

	Global CurrentLevel:Level
		
	' Handles -- automatically reset...
	
	Global Player:Rocket

	Method New ()

		CurrentLevel.Number = 0

		Score = 0
		CurrentLevel.Gravity = 0.025
		
		' Create player with default image, middle of screen, just off bottom, scale 0.25 x 0.25...
		
		Player			= New Rocket (DEFAULT_PLAYER, VDeviceWidth / 2, VDeviceHeight + 64, 1.0, 1.0)'0.25, 0.25)
		Timer.NewGame		= New Timer
		
	End

	' -------------------------------------------------------------------------
	' Used to set game state...
	' -------------------------------------------------------------------------
	
	Method SetState (state:Int)
	
		GameSession.GameState = state
		
		Select GameSession.GameState
		
			Case STATE_MENU
				'Print "Click mouse to start"
			
			Case STATE_LOADLEVEL
				' Nothing...
			
			Case STATE_PLAYING
				'Print "Press ESC to exit or P to pause"
			
			Case STATE_PAUSED
				'Print "Press P or ESC to continue"
			
			Case STATE_GAMEOVER
				Timer.NewGame.Reset
				
'			Default
'				Print "ERROR: Unknown game state!"
		
		End
	
	End
	
End
