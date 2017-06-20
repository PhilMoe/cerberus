
Import rockout

Function UpdateGame ()

		' ---------------------------------------------------------------------
		' Mouse position in virtual display, used universally...
		' ---------------------------------------------------------------------
		
		RockOut.mx = VMouseX ()
		RockOut.my = VMouseY ()
		
		' ---------------------------------------------------------------------
		' Run appropriate update code for each game state...
		' ---------------------------------------------------------------------
		
		Select GameSession.GameState
		
			' -----------------------------------------------------------------
			' In menu...
			' -----------------------------------------------------------------
			
			Case STATE_MENU

				If RockOut.temp.Clicked (RockOut.mx, RockOut.my)
					GameSession.SetState STATE_STARTGAME
				Endif
			
			Case STATE_STARTGAME
			
				GameSession = New Session
			
				GameSession.SetState STATE_LOADLEVEL
				
			' -----------------------------------------------------------------
			' Load level data...
			' -----------------------------------------------------------------
			
			Case STATE_LOADLEVEL
			
				GameSession.CurrentLevel.LoadLevel
				GameSession.SetState STATE_PLAYING
								
			' -----------------------------------------------------------------
			' In game...
			' -----------------------------------------------------------------
			
			Case STATE_PLAYING

				' -------------------------------------------------------------
				' Update shots and blocks...
				' -------------------------------------------------------------
			
				Shot.UpdateAll
				Block.UpdateAll
				ScoreBubble.UpdateAll
				
				' -------------------------------------------------------------
				' Get keyboard input and move player...
				' -------------------------------------------------------------

				' NB. Mouse input obtained from mx/my fields...
				
				CheckKeys
				
				GameSession.Player.UpdatePlayer STATE_PLAYING, KeyDown (KEY_LMB) ' Keeps input here
				
				' -------------------------------------------------------------
				' Check for all sprite collisions...
				' -------------------------------------------------------------

				CheckCollisions
				
				' -------------------------------------------------------------
				' Is player alive?
				' -------------------------------------------------------------

				If Not GameSession.Player.Alive
					GameSession.SetState STATE_GAMEOVER
				Else
					If LevelComplete
						GameSession.SetState STATE_LOADLEVEL
					Endif
				Endif
				
			' -----------------------------------------------------------------
			' In paused state...
			' -----------------------------------------------------------------
			
			Case STATE_PAUSED
			
				If KeyHit (KEY_ESCAPE) or KeyHit (KEY_P)
					GameSession.SetState STATE_PLAYING
				Endif
				
			' -----------------------------------------------------------------
			' On 'game over' screen...
			' -----------------------------------------------------------------

			Case STATE_GAMEOVER
			
				' -------------------------------------------------------------
				' Only allow new game after short delay...
				' -------------------------------------------------------------

				For Local b:Block = Eachin GameSession.CurrentLevel.Blocks
					b.Fall Rnd (-4, 4), Rnd (-8)
				Next				

				Shot.UpdateAll
				Block.UpdateAll
				ScoreBubble.UpdateAll
				
				GameSession.Player.UpdatePlayer STATE_GAMEOVER ' Includes mouse position...

				If Timer.NewGame.TimeOut (DELAY_NEWGAME)

					If KeyHit (KEY_LMB)
						GameSession.SetState STATE_MENU
					Endif

				Endif
				
			' -----------------------------------------------------------------
			' Undefined state...
			' -----------------------------------------------------------------
			
'			Default
'				Print "Undefined state!"
		
		End

End

Function LevelComplete ()
	If GameSession.CurrentLevel.Blocks.IsEmpty Then Return True
End
