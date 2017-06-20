
Import rockout

Class Level

	Global Number:Int
	Global Name:String
	Global StartLine:Int
	Global Graphics:String
	
	Global Shots:List <Shot>

	Global Blocks:List <Block>
	Global FallingBlocks:List <Block>
	
	Global ScoreBubbles:List <ScoreBubble>

	Global Gravity:Float

	Method New ()
	
		Shots				= New List <Shot>
		Blocks			= New List <Block>
		FallingBlocks		= New List <Block>
	
		ScoreBubbles		= New List <ScoreBubble>
	
		Timer.ShotReload		= New Timer

		Number = Number + 1
		Name = "Level " + Number
		
	End

	' TEMP HACK LEVEL!
	
	Function LoadLevel:Level ()

		Local level:Level = New Level
		
		Local levelstring:String = LoadString ("level" + level.Number + ".txt")

		If levelstring
			
			Local leveldata:String [] = levelstring.Split ("~n")
			
			Local linecount:Int
			
			For Local line:String = Eachin leveldata
			
				linecount = linecount + 1
				
				'Print "Line " + linecount + ": " + line
				
				If line.ToLower.StartsWith ("start")
					Level.StartLine = linecount
				Endif

				If line.ToLower.StartsWith ("gfx")
					Level.Graphics = "TEMP"
				Endif
				
				' |01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01| (Length = 61)
				
				' Test blocks...
				
				For Local temp:Int = 0 To 2
					New Block (DEFAULT_BLOCK, Rnd (VDeviceWidth), Rnd (VDeviceHeight) * 0.5, 0, 0, 1.0, 1.0)
				Next
				
			Next
			
			If GameSession.CurrentLevel.Blocks.IsEmpty Then Error "No blocks found in level " + level.Number + "!"
			
			Return level
			
		Else
			Error "No new level found!"
		Endif
			
	End
	
End




















