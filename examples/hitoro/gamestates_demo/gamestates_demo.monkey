
' Game states demo...

' LEFT-CLICK to PLAY
' P to PAUSE/UNPAUSE
' ESC to return to MENU

Import mojo

Global Game:RocketGame

' Constants used to set/track game state...

Const STATE_MENU		= 1
Const STATE_PLAYING		= 2
Const STATE_PAUSED		= 3

Class Rocket

	  Field image:Image

	  Field x:Float
	  Field y:Float

	  Field mousediv:Float = 12

	  Method MovePlayer ()

			Local xdist:Float = Game.mx - x
			Local ydist:Float = Game.my - y

			Local xstep:Float = xdist / mousediv
			Local ystep:Float = ydist / mousediv

			x = x + xstep
			y = y + ystep

	  End

End

Class RocketGame Extends App

	  Field mx:Float
	  Field my:Float

	  Field player:Rocket

	  Field menu:Image

	  ' Stores current game state...

	  Global GameState:Int = STATE_MENU

	  ' Used to set game state...

	  Method SetState (state:Int)

   			 GameState = state

	  		 Select GameState

				   Case STATE_MENU
				   		Print "Click mouse to start"

				   Case STATE_PLAYING
					   Print "Press ESC to exit or P to pause"

				   Case STATE_PAUSED
					   Print "Press P or ESC to continue"

				   Default
					   Print "ERROR: Unknown game state!"

	  		 End

	  End

	  Method OnCreate ()

	  		 player = New Rocket

	  		 player.image = LoadImage ("player.png")
			 player.image.SetHandle player.image.Width () * 0.5, player.image.Height () * 0.5

			 menu = LoadImage ("menu.png")

	  		 SetUpdateRate 60

	  End

	  Method OnUpdate ()

	  		mx = MouseX ()
	  		my = MouseY ()

			' Run appropriate update code for each game state...

			Select GameState

				   ' In menu...

				   Case STATE_MENU

				   		If KeyHit (KEY_LMB)
				   		   SetState STATE_PLAYING
				   		Endif

				   ' In game...

				   Case STATE_PLAYING

				   		player.MovePlayer

				   		If KeyHit (KEY_ESCAPE)
				   		   SetState STATE_MENU
				   		Endif

				   		If KeyHit (KEY_P)
				   		   SetState STATE_PAUSED
				   		Endif

				   ' In paused state...

				   Case STATE_PAUSED

				   		If KeyHit (KEY_ESCAPE) Or KeyHit (KEY_P)
				   		   SetState STATE_PLAYING
				   		Endif

			End

	  End

	  Method OnRender ()

			' Run appropriate drawing code for each game state...

			Select GameState

				   Case STATE_MENU

				   		Cls 32, 64, 128
						DrawImage menu, (DeviceWidth () * 0.5) - (menu.Width () * 0.5), (DeviceHeight () * 0.5) - (menu.Height () * 0.5)

				   Case STATE_PLAYING

				   		Cls 32, 64, 128
						DrawImage player.image, player.x, player.y, 0, 0.25, 0.25

				   Case STATE_PAUSED

				   		Cls 32, 64, 128
						DrawImage player.image, player.x, player.y, 0, 0.25, 0.25

				   Default
				   		Cls 255, 0, 0 ' Unknown state -- red screen of death!

			End

	  End

End

' Here we go!

Function Main ()
		 Game = New RocketGame
		 Print "Click mouse to start"
End
