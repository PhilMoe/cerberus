
Import rockout

' TODO: Separate list for indestructable blocks?

Class Block Extends Sprite

	Field parent:List <Block>
	
	Field hittime:Int
	Field hitcount:Int
	
	Field hitby:Shot	' Shot which hit this block; gets speed from here
	Field hitx:Float	' x-position when hit
	Field hity:Float	' y-position when hit
	
	Field strength:Int = 1
	
	' Update parent via go-between function?
	
	Method Fall (forcexs:Float = 0.0, forceys:Float = 0.0)
	
		GameSession.CurrentLevel.FallingBlocks.AddLast Self
		Self.parent = GameSession.CurrentLevel.FallingBlocks
		
		GameSession.CurrentLevel.Blocks.RemoveEach Self
		
		If Self.hitby
			Self.xs = Self.hitby.xs * 0.5
			Self.ys = -Self.hitby.ys * 0.5
		Else
			Self.xs = forcexs
			Self.ys = forceys
		Endif
		
	End
	
	Method Delete ()
		parent.RemoveEach Self
	End
	
	Method New (img:Image, x:Float, y:Float, xs:Float, ys:Float, xscale:Float, yscale:Float)

		Self.image = img

		Self.x = x
		Self.y = y

		Self.xscale = xscale
		Self.yscale = yscale

		' This is the pixel width of the image after scaling...
		
		Self.width = img.Width * xscale
		Self.height = img.Height * yscale

		GameSession.CurrentLevel.Blocks.AddLast Self

		Self.parent = GameSession.CurrentLevel.Blocks
		
	End

	Function UpdateAll ()
	
		Local b:Block

		For b = Eachin GameSession.CurrentLevel.FallingBlocks

			b.x = b.x + FrameScale (b.xs)

			b.ys = b.ys + FrameScale (GameSession.CurrentLevel.Gravity) * 2.0
			b.y = b.y + FrameScale (b.ys)

			If b.y > VDeviceHeight + b.height Then b.Delete

		Next

	End

	Function Render ()

		Local b:Block

		For b = Eachin GameSession.CurrentLevel.Blocks
			b.Draw
		Next

		For b = Eachin GameSession.CurrentLevel.FallingBlocks
			b.Draw
		Next

	End

End
