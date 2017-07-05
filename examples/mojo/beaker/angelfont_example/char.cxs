Import mojo.graphics

Class Char
	Field asc:Int
	Field x:Int
	Field y:Int
	
	Field width:Int
	Field height:Int = 0
	
	Field xOffset:Int = 0
	Field yOffset:Int = 0
	Field xAdvance:Int = 0
	
	Field page:Int = 0
	
	
	Method New(x:Int,y:Int, w:Int, h:Int, xoff:Int=0, yoff:Int=0, xadv:Int=0, page:Int=0)
		Self.x = x
		Self.y = y
		Self.width = w
		Self.height = h
		
		Self.xOffset = xoff
		Self.yOffset = yoff
		Self.xAdvance = xadv
		Self.page = page
	End
	
	Method Draw(fontImage:Image, linex,liney)
		DrawImageRect(fontImage, linex+xOffset,liney+yOffset, x,y, width,height)
	End Method

	Method toString:String()
		Return String.FromChar(asc)+"="+asc
	End Method
End Class






