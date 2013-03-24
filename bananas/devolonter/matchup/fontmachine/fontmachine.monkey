#rem
header:This module is part of the FontMachine library. 
The FontMachine library provides a complete bitmap font system for its use in the Monkey programming language.

This library contains the following modules:
[list]
[*][a bitmapchar.monkey.html]bitmapchar[/a]
[*][a bitmapcharmetrics.monkey.html]bitmapcharmetrics[/a]
[*][a bitmapfont.monkey.html]bitmapfont[/a]
[*][a drawingpoint.monkey.html]drawingpoint[/a]
[*][a drawingrectangle.monkey.html]drawingrectangle[/a]
[*][a edrawalign.monkey.html]eDrawAlign[/a]
[*][a fontinterface.monkey.html]fontinterface[/a]
[*]fontmachine
[/list]
This module allows you to load bitmapfonts and draw them in your games or applications very easilly.
This module requires the Mojo library for monkey.
#end
'This is the fontmachine module for the Mojo.graphics framework
Import fontmachine.bitmapfont

'summary: This constant contains the library version
Const Version:String = "12.04.22-A"

'summary: This constant contains the library name
Const Name:String = "FontMachine"

#rem
footer:This FontMachine library is released under the MIT license:
[quote]Copyright (c) 2011 Manel Ibanez

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
[/quote]

Additionally, this is the module changelog:
[quote]
Version 12.07.19-A
[list]
[*]Improved text rendering speed on HTML5.
[*]Improved lettering space calculation
[/list]
Version 12.04.22-A
[list]
[*]Fixed a bug that was ignoring last character when calculating the metrics of a string, and the last character was a SPACE character.
[*]Fixed a broke link in the FontMachine interface documentation.
[*]Added a module signature file so the module can be automatically updated from Jungle Ide.
[/list]
Version 12.02.20-A
[list]
[*]Fixed an small bug regarding Font interface consitency. Text width should be Float intead of Int.
[/list]Version 12.02.17-A
[list]
[*]Fixed an small aligment bug on multiline strings rendering
[/list]
Version 12.02.16-A
[list]
[*]Implemented several optimizations on text drawing routines, so they generate less garbage. (thanks to sgg for the suggestion and code samples)
[*]Implemented a new method on the BitmapFont class called Charcount and returns the number of BitMapChar objects contained available in a given font, so you can iterate throug them properly.
[*]Fixed an issue that was making the whole library to cause a crash when a GetInfo command was performed on a null BitMapChar.
[/list]
Version 12.02.15-B
[list]
[*]Implemented a Kerning property for all bitmap fonts. X and Y values will define additional horizontal and vertical font kerning
[*]Implemented a GetTxtHeight function that returns the height of a given string, in graphic units, taking into account multiline strings
[*]Improved the GetTxtWidth command in a way that it now handles properly multiline strings
[*]B Version: Fixed some small issues on text size calculation
[/list]
Version 12.01.27-A
[list]
[*]Fixed a compatibility issue with the Font interface
[/list]
Version 12.01.25-A
[list]
[*]Implemented the aligment flags on draw operations
[*]Fixed a syntax error in several GetInfo methods for the bitmapfont class
[*]Fixed an incompatibility with the latest Monkey compiler, due to abstract identifier inside the Font interface
[*]Addeed the aligment enumerator-like class
[*]Optimization of the DrawText command on Java based targets, such as Android (Thanks to SGG at [a JungleIde.com]Jungle Ide[/a] for this one!)
[/list]
Version 11.08.03-A
[list]
[*]Implemented single texture fonts support
[*]Optimized fonts loading time
[*]Reduced slightly memory used by each Font instance
[/list]
[/quote]

#end