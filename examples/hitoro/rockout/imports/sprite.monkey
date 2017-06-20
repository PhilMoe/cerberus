
Import rockout

' Sprite wraps image and associated information...

Class Sprite

	Field image:Image
	
	Field x:Float
	Field y:Float

	Field width:Float
	Field height:Float
	
	' Speed (pixels per frame)...
	
	Field xs:Float
	Field ys:Float
	
	' Drawing scale...
	
	Field xscale:Float = 1.0
	Field yscale:Float = 1.0

	' Drawing angle...
	
	Field rotation:Float = 0.0
	
	Method Draw ()
		DrawImage image, x, y, rotation, xscale, yscale
	End

End
