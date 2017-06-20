#rem
header:This module is part of the FontMachine library. 
The FontMachine library provides a complete bitmap font system for its use in the Monkey programming language.

This module contains a base-class with all available aligment constants
#end

#rem
	summary:This abstract class contains all available aligment flags
#end
Class eDrawAlign abstract
	'summary: Use this constant for left aligment of text on draw operations.
	Const LEFT:Int = 1
	'summary: Use this constant for centered aligment of text on draw operations.
	Const CENTER:Int = 2
	'summary: Use this constant for right aligment of text on draw operations.
	Const RIGHT:Int = 3
End
#rem
footer:This FontMachine library is released under the MIT license:
[quote]Copyright (c) 2011 Manel Ibanez

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
[/quote]
#end