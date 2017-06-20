#rem
	header:This module contains the DrawingRectangle class.
	This class is a simple X and Y vector, with additional Width and Height
#end
Import fontmachine.drawingpoint 
#rem
	summary: This class contains the math representation of a rectangle.
#end
Class DrawingRectangle 
	'summary: This field contains the X location of this point.
	Field x:Float
	'summary: This field contains the Y location of this point.
	Field y:Float
	'summary: This is the width representation of the DrawingRectangle class
	Field width:Int
	'summary: This is the height representation of the DrawingRectangle class
	Field height:Int
	'summary: This method returns a string with a representation of the rectangle coordinates.
	Method DebugString:String() ; Return "(" + x + ", " + y + ", " + width + ", " + height +  ")" ;	End
End Class 


#rem
footer:This FontMachine library is released under the MIT license:
[quote]Copyright (c) 2011 Manel Ibanez

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
[/quote]
#end