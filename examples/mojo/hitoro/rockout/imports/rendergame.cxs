
Import rockout

Function DrawCursor ()
	SetColor 255, 255, 255
	DrawLine RockOut.mx - 8, RockOut.my, RockOut.mx + 8, RockOut.my
	DrawLine RockOut.mx, RockOut.my - 8, RockOut.mx, RockOut.my + 8
End

Function RenderGame ()

	Cls 32, 64, 128

	Block.Render
	Shot.Render
	ScoreBubble.Render

	DrawCursor
	
	GameSession.Player.Render
	
	DrawText "Shields: " + GameSession.Player.Shields, 20, 20
	DrawText "Score:   " + GameSession.Score, 20, 40
	DrawText "Level: " + GameSession.CurrentLevel.Name, 20, 80
	
	DrawText "Use LEFT/RIGHT and ENTER to zoom", 20, 60
	
	DrawText "Update rate (UP/DOWN to change): " + UPDATE_RATE, 20, VDeviceHeight - 40
	
End

Function RenderStates ()

	' ---------------------------------------------------------------------
	' Scale all drawing to viewport...
	' ---------------------------------------------------------------------

	UpdateVirtualDisplay ' From imports.virtual
	
	' ---------------------------------------------------------------------
	' Run appropriate drawing code for each game state...
	' ---------------------------------------------------------------------

	Select GameSession.GameState
	
		' -----------------------------------------------------------------
		' In menu...
		' -----------------------------------------------------------------

		Case STATE_MENU
		
			Cls 32, 64, 128
		'	Local click:String = "Click mouse to start..."
		'	DrawText click, CenterStringX (click), CenterStringY (click)
		
			RockOut.temp.Draw

			DrawCursor

		' -----------------------------------------------------------------
		' Loading level...
		' -----------------------------------------------------------------

		Case STATE_LOADLEVEL
		
'				Print "Level loaded!"
		
		' -----------------------------------------------------------------
		' Game playing...
		' -----------------------------------------------------------------

		Case STATE_PLAYING
		
			RenderGame
		
'			Scale 2, 2
'			DrawImage RockOut.shade, 0, 0
			
		' -----------------------------------------------------------------
		' Game paused...
		' -----------------------------------------------------------------

		Case STATE_PAUSED
		
			RenderGame
			CenterText "Paused - Press P to continue"
			
		' -----------------------------------------------------------------
		' Game over screen...
		' -----------------------------------------------------------------

		Case STATE_GAMEOVER
		
			RenderGame
			CenterText "Game over! Click to play again..."

			DrawCursor
			
		' -----------------------------------------------------------------
		' Unknown game state...
		' -----------------------------------------------------------------

		Default
		
			Cls 1.0, 0.0, 0.0 ' Unknown state -- red screen of death!
	
	End
	
'	ShowState

End
