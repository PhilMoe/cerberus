
' -----------------------------------------------------------------------------
' RockOut!
' -----------------------------------------------------------------------------



' **** Now refresh-rate independent as of 6 Feb 2011! ****



' P to PAUSE/UNPAUSE
' ESC to return to MENU



' TODO: Put up 'click here' between levels, make menu call same routine...
' TODO: Allow zoom controls in non-game states!




' -----------------------------------------------------------------------------
' IMPORTS...
' -----------------------------------------------------------------------------

' -----------------------------------------------------------------------------
' monkey's mojo...
' -----------------------------------------------------------------------------

Import mojo

' -----------------------------------------------------------------------------
' Game-specific bits...
' -----------------------------------------------------------------------------

Import imports.media			' Default media filenames
Import imports.defaultmedia		' Default media handles

Import imports.game			' Main game logic ' IMPORTS GAME, LEVEL, etc.

Import imports.sprite			' Graphical objects
Import imports.rocket			' Player (rocket) object
Import imports.shot			' Shot object
Import imports.block			' Block object
Import imports.scorebubble		' Score bubbles

Import imports.constants		' Various game constants
Import imports.functions		' Miscellaneous functions
Import imports.collisions		' Collision functions

Import imports.autofit			' Virtual display stuff
Import imports.button			' GUI buttons
Import imports.timer			' Timer object and all timers

' -----------------------------------------------------------------------------
' Game/app instance...
' -----------------------------------------------------------------------------

Global RockOut:GameApp				' Application control
' GameSession:Session				' Game session control
' GameSession.CurrentLevel:Level		' Game level control

' -----------------------------------------------------------------------------
' Here we go!
' -----------------------------------------------------------------------------

Function Main ()
	RockOut = New GameApp (854, 480)
End

' Regarding weird screen size: chose "FWVGA" to allow for 16:9 display without
' being too big for lower-powered devices...

' http://en.wikipedia.org/wiki/List_of_common_resolutions
' http://en.wikipedia.org/wiki/Wide_VGA#FWVGA_.28480p.29
