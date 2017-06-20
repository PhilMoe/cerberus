
Import mojo

' The overall Game object, handling loading, mouse position, high-level game control and rendering...

Class RocketGame Extends App

	  Field mx:Float								' Mouse x-position
	  Field my:Float								' Mouse y-position

	  Field player:Rocket							' Player object handle

	  ' Stuff to do on startup...

	  Method OnCreate ()

	  		 ' Create a player object and assign to handle...

	  		 player = New Rocket

	  		 ' Load player image and set its handle to middle of image...

	  		 player.image = LoadImage ("player.png", 1, Image.MidHandle)

			 ' 60 frames per second, please!

	  		 SetUpdateRate 60

	  End

	  ' Stuff to do while running...

	  Method OnUpdate ()

	  		mx = MouseX ()						' Store mouse x-position
	  		my = MouseY ()						' Store mouse y-position

			player.MovePlayer mx, my				' Call player object's MovePlayer method (move towards mouse)

	  End

	  ' Drawing code...

	  Method OnRender ()

	  		 Cls 32, 64, 128						' Clear screen

			 ' Draw player image at player's x/y position, no rotation, scaled to 0.25 horizontally and vertically...

			 DrawImage player.image, player.x, player.y, 0, 0.25, 0.25

	  End

End

' Player object definition...

Class Rocket

	  Field image:Image							' Player image

	  Field x:Float								' Player x-position
	  Field y:Float								' Player y-position

	  Field mousediv:Float = 12						' Mouse smoothing value (trial and error!)

	  ' This method handles the mouse smoothing, moving the player towards the mouse position...

	  Method MovePlayer (towardsx:Float, towardsy:Float)

			Local xdist:Float = towardsx - x			' Distance from mouse position to current player position
			Local ydist:Float = towardsy - y			' Ditto

			Local xstep:Float = xdist / mousediv		' Distance divided by value in mousediv field
			Local ystep:Float = ydist / mousediv		' Ditto

			x = x + xstep						' Move player by this distance
			y = y + ystep						' Ditto

	  End

End

' Here we go!

Function Main ()
	New RocketGame								' RocketGame extends App, so monkey will handle running from here...
End
