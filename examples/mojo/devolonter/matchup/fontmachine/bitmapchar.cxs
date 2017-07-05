#rem
	header: This module contains the BitMapChar class.
	This class represent a character in a BitmapFont.
#end
Import bitmapcharmetrics 
Import mojo.graphics 


#rem
	summary: This class represents a font character.
	This class represents a font character and provides methods to load and unload the character images on dynamic fonts, and provide methods to get the location of the char in the packed texture on packed fonts.
	Any character in any font, is an instance of this class.
	Beaware that this font represent a character layer. That is, each character is a Face character, a Border character or a Shadow character.
#end
Class BitMapChar 
	'summary: This field contains the drawing metrics information of the character. That is, width, height, space to next character, etc.
	Field drawingMetrics:= new BitMapCharMetrics
	'summary: This field contains the character image on dynamic fonts.
	Field image:Image
	'summary: This field contains the texture index on packed fonts. (advanced use)
	Field packedFontIndex:int
	'summary: This field contains the X and Y offset of the character in the packed texture, on non dynamic fonts.
	field packedPosition:drawingpoint.DrawingPoint = new drawingpoint.DrawingPoint
	'summary: This field contains the width and height offset of the character in the packed texture, on non dynamic fonts.
	Field packedSize:drawingpoint.DrawingPoint = new drawingpoint.DrawingPoint
	
	'summary: This method will force a dynamic font to load the character image to VRam.
	Method LoadCharImage()
		'if imageResourceName = null Then return
		if CharImageLoaded() = false then
			image = LoadImage(imageResourceName)
			image.SetHandle(-self.drawingMetrics.drawingOffset.x,-self.drawingMetrics.drawingOffset.y)
			imageResourceNameBackup = imageResourceName
			imageResourceName = ""
		endif
	End Method
	#rem
		summary: This method will return true or false if the character image has been loaded to VRam on dynamic fonts.
		Notice that this method will return always FALSE for packed fonts.
	#end
	Method CharImageLoaded:Bool()
		if image = null And imageResourceName <> "" then Return False Else Return true
	End Method
	
	Method SetImageResourceName(value:String)
		imageResourceName = value
	End Method
	
	#rem
		summary: This method will force a dynamic font to unload the character image from VRam.
	#end
	Method UnloadCharImage()
		if CharImageLoaded() = True Then
			image.Discard()
			image = null
			imageResourceName = imageResourceNameBackup
			imageResourceNameBackup = ""
		EndIf 
	End Method
	Private
	Field imageResourceNameBackup:string
	Field imageResourceName:String = ""

End Class

#rem
footer:This FontMachine library is released under the MIT license:
[quote]Copyright (c) 2011 Manel Ibanez

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
[/quote]
#end