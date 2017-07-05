
Import rockout

' Temp rendering as text!

Class ScoreBubble Extends Sprite
	
	Field score:Int

	Field parent:List <ScoreBubble>
	
	Method New (x:Float, y:Float, score:Int)
		Self.x = x
		Self.y = y
		Self.ys = 1
		Self.score = score
		GameSession.CurrentLevel.ScoreBubbles.AddLast Self
		Self.parent = GameSession.CurrentLevel.ScoreBubbles
	End

	Method Delete ()
		parent.RemoveEach Self
	End
	
	Function UpdateAll ()
	
		For Local score:ScoreBubble = Eachin GameSession.CurrentLevel.ScoreBubbles
			score.ys = score.ys - FrameScale (GameSession.CurrentLevel.Gravity) * 4.0
			score.y = score.y + FrameScale (score.ys)
			If score.y < -FONT_HEIGHT Then score.Delete
		Next
		
	End

	Function Render ()
		For Local score:ScoreBubble = Eachin GameSession.CurrentLevel.ScoreBubbles
			DrawText score.score, score.x, score.y
		Next
	End

End
