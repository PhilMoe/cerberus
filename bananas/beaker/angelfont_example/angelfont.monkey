#TEXT_FILES="*.txt|*.xml|*.json|*.fnt"

Strict

Import mojo

Import char
Import kernpair

Import config	'thanks to skn3 - you can find the latest version of this file in bananas/skn3/config folder.

'Bitmap font tools:
'PC: http://www.angelcode.com/products/bmfont/
'Mac/PC: http://slick.cokeandcode.com/demos/hiero.jnlp
'Mac (paid): http://www.bmglyph.com/ - cheapest
'Mac (paid): http://glyphdesigner.71squared.com/ - best

'For more information about AngelFont look here: http://www.monkeycoder.co.nz/Community/posts.php?topic=141


Class AngelFont
	Private
	
	Global _list:StringMap<AngelFont> = New StringMap<AngelFont>
	
	Field image:Image[] = New Image[1]	
'	Field blockSize:Int
	Field chars:Char[256]
	
'	Field kernPairs:StringMap<KernPair> = New StringMap<KernPair>
	Field kernPairs:IntMap<IntMap<KernPair>> = New IntMap<IntMap<KernPair>>
	Global firstKp:IntMap<KernPair>
	Global secondKp:KernPair
	
'	Field section:String
	Field iniText:String

	Field xOffset:Int
	Field yOffset:Int
	
	Field prevMouseDown:Bool = False

	Public
	Const ALIGN_LEFT:Int = 0
	Const ALIGN_CENTER:Int = 1
	Const ALIGN_RIGHT:Int = 2
	Const ALIGN_FULL:Int = 3
	
	Global current:AngelFont
	Global err:String
	
	Field name:String
	Field useKerning:Bool = True

	Field lineGap:Int = 5
	Field height:Int = 0
	Field heightOffset:Int = 9999
	Field scrollY:Int = 0
	
	Field italicSkew:Float = 0.25
	
	Method New(url:String="")
		If url <> ""
			Self.LoadFont(url)
			Self.name = url
			_list.Insert(url,Self)
		Endif
	End Method
	
	Method GetChars:Char[]()
		Return chars
	End

	Method LoadFont:Void(url:String)			'deprecated
		
		err = ""
		current = Self
		iniText = LoadString(url+".txt")
		Local lines:= iniText.Split(String.FromChar(10))
		For Local line:= Eachin lines
		
			line=line.Trim()
			
			If line.StartsWith("id,") Or line = "" Continue
			If line.StartsWith("first,")
'				kernPairs = New StringMap<KernPair>
				Continue
			Endif
			Local data$[] = line.Split(",")
			
			For Local i:=0 Until data.Length
				data[i]=data[i].Trim()
			Next
			
			err += data.Length+","	'+"-"+line
			If data.Length > 0
				If data.Length = 3
	'				kerning.Insert(data[0]+"_"+data[1], New Kerning(Int(data[0]), Int(data[1]), Int(data[2]))
'					kernPairs.Add(String.FromChar(Int(data[0]))+"_"+String.FromChar(Int(data[1])), New KernPair(Int(data[0]), Int(data[1]), Int(data[2])))
'					kernPairs.Add(Int(data[0])*10000+Int(data[1]), New KernPair(Int(data[0]), Int(data[1]), Int(data[2])))
					
					Local first:Int = Int(data[0])
					firstKp = kernPairs.Get(first)
					If firstKp = Null
						kernPairs.Add(first, New IntMap<KernPair>)
						firstKp = kernPairs.Get(first)
					End
					
					Local second:Int = Int(data[1])
					
					firstKp.Add(second, New KernPair(Int(data[0]), Int(data[1]), Int(data[2])))
					
				Else
					If data.Length >= 8
						chars[Int(data[0])] = New Char(Int(data[1]), Int(data[2]), Int(data[3]), Int(data[4]),  Int(data[5]),  Int(data[6]),  Int(data[7]), Int(data[8]))
						Local ch := chars[Int(data[0])]
						If ch.height > Self.height Self.height = ch.height
						If ch.yOffset < Self.heightOffset Self.heightOffset = ch.yOffset
		'				ch.asc = Int(data[0])
					Endif
				Endif
			Endif
		Next
		
		image[0] = LoadImage(url+".png")
	End Method
	
	Method LoadFontXml:Void(url:String)
		current = Self
		
		iniText = LoadString(url+".fnt")
		Local lines:String[] = iniText.Split(String.FromChar(10))
		Local firstLine:String = lines[0]
'		Print "lines count="+lines.Length
		If firstLine.Contains("<?xml")
			Local lineList:List<String> = New List<String>(lines)
			lineList.RemoveFirst()
			lines = lineList.ToArray()
			iniText = "~n".Join(lines)
		End	
		
		
		Local pageCount:Int = 0
		
		Local config:= LoadConfig(iniText)
		
		Local nodes := config.FindNodesByPath("font/chars/char")
		For Local node := Eachin nodes
'			Print " -> "+node.GetName()+"(id="+node.GetAttribute("id")+" x="+node.GetAttribute("x")+" y="+node.GetAttribute("y")+" width="+node.GetAttribute("width")+" height="+node.GetAttribute("height")+" )"
			'Print " -> "+node.GetName()+" = "+node.GetValue()
			Local id:Int = Int(node.GetAttribute("id"))
			Local page:Int = Int(node.GetAttribute("page"))
			If pageCount < page pageCount = page
			chars[id] = New Char(Int(node.GetAttribute("x")), Int(node.GetAttribute("y")), Int(node.GetAttribute("width")), Int(node.GetAttribute("height")),  Int(node.GetAttribute("xoffset")),  Int(node.GetAttribute("yoffset")),  Int(node.GetAttribute("xadvance")), page)
			Local ch := chars[id]
			If ch.height > Self.height Self.height = ch.height
			If ch.yOffset < Self.heightOffset Self.heightOffset = ch.yOffset
		Next
		
		nodes = config.FindNodesByPath("font/kernings/kerning")
		For Local node := Eachin nodes
'			Local first:String = String.FromChar(Int(node.GetAttribute("first")))
'			Local second:String = String.FromChar(Int(node.GetAttribute("second")))
			Local first:Int = Int(node.GetAttribute("first")) '* 10000
			firstKp = kernPairs.Get(first)
			If firstKp = Null
				kernPairs.Add(first, New IntMap<KernPair>)
				firstKp = kernPairs.Get(first)
			End
			
			Local second:Int = Int(node.GetAttribute("second"))
			
'			kernPairs.Add(first+"_"+second, New KernPair(Int(first), Int(second), Int(node.GetAttribute("amount"))))
'			kernPairs.Add(first+second, New KernPair(first, second, Int(node.GetAttribute("amount"))))
			firstKp.Add(second, New KernPair(first, second, Int(node.GetAttribute("amount"))))
			'Print "adding kerning "+ String.FromChar(first)+" "+String.FromChar(second)
		End
		
		If pageCount = 0
			image[0] = LoadImage(url+".png")
			If image[0] = Null image[0] = LoadImage(url+"_0.png")
		Else
			For Local page:= 0 To pageCount
				If image.Length < page+1 image = image.Resize(page+1)
				image[page] = LoadImage(url+"_"+page+".png")
			End
		End					
'		Print iniText
	End
	
	
	Method Use:Void()
		current = Self
	End Method
	
	Function GetCurrent:AngelFont()
		Return current
	End
	
#rem	
	Function Use:AngelFont(name:String)
		For Local af := Eachin _list
			If af.name = name
				current = af
				Return af
			End
		Next
		Return Null
	End
#end
	
	Method DrawItalic:Void(txt$,x#,y#)
		Local th#=TextHeight(txt)
		
		PushMatrix
			Transform 1,0,-italicSkew,1, x+th*italicSkew,y
			DrawText txt,0,0
		PopMatrix		
	End 
	
	Method DrawBold:Void(txt:String, x:Int, y:Int)
		DrawText(txt, x,y)
		DrawText(txt, x+1,y)
	End
	
	
	Method DrawText:Void(txt:String, x:Int, y:Int)
'		Local prevChar:String = ""
		Local prevChar:Int = 0
		xOffset = 0
		
		For Local i:= 0 Until txt.Length
			Local asc:Int = txt[i]
			Local ac:Char = chars[asc]
'			Local thisChar:String = String.FromChar(asc)
			Local thisChar:Int = asc
			If ac  <> Null
				If useKerning
					firstKp = kernPairs.Get(prevChar)
					If firstKp <> Null
						secondKp = firstKp.Get(thisChar)
						If secondKp <> Null
							xOffset += secondKp.amount
'							Print prevChar+","+thisChar
						End
					Endif
				Endif
				ac.Draw(image[ac.page], x+xOffset,y)
				xOffset += ac.xAdvance
				prevChar = thisChar
			Endif
		Next
	End Method
	
	Method DrawText:Void(txt:String, x:Int, y:Int, align:Int)
		xOffset = 0
		Select align
			Case ALIGN_CENTER
				DrawText(txt,x-(TextWidth(txt)/2),y)
			Case ALIGN_RIGHT
				DrawText(txt,x-TextWidth(txt),y)
			Case ALIGN_LEFT
				DrawText(txt,x,y)
		End Select
	End Method

	Method DrawHTML:Void(txt:String, x:Int, y:Int)
'		Local prevChar:String = ""
		Local prevChar:Int = 0
		xOffset = 0
		Local italic:Bool = False
		Local bold:Bool = False
		Local th#=TextHeight(txt)
		
		For Local i:= 0 Until txt.Length
			'err += txt[i..i+1]
			
			While txt[i..i+1] = "<"
				Select txt[i+1..i+3]
					Case "i>"
						italic = True
						i += 3
					Case "b>"
						bold = True
						i += 3
					Default
						Select txt[i+1..i+4]
							Case "/i>"
								italic = False
								i += 4
							Case "/b>"
								bold = False
								i += 4
							Default
								i += 1
						End
				End
				If i >= txt.Length
					Return
				End
			Wend
			Local asc:Int = txt[i]
			Local ac:Char = chars[asc]
'			Local thisChar:String = String.FromChar(asc)
			Local thisChar:Int = asc
			If ac  <> Null
				If useKerning
					firstKp = kernPairs.Get(prevChar)
					If firstKp <> Null
						secondKp = firstKp.Get(thisChar)
						If secondKp <> Null
							xOffset += secondKp.amount
'							Print prevChar+","+thisChar+"  "+String.FromChar(prevChar)+","+String.FromChar(thisChar)
						End							
					Endif
				Endif
				If italic = False
					ac.Draw(image[ac.page], x+xOffset,y)
					If bold
						ac.Draw(image[ac.page], x+xOffset+1,y)
					End
				Else
					PushMatrix
						Transform 1,0,-italicSkew,1, (x+xOffset)+th*italicSkew,y
						ac.Draw(image[ac.page], 0,0)
						If bold
							ac.Draw(image[ac.page], 1,0)
						Endif					
					PopMatrix		
				End	
				xOffset += ac.xAdvance
				prevChar = thisChar
			Endif
		Next
	End Method
	
	Method DrawHTML:Void(txt:String, x:Int, y:Int, align:Int)
		xOffset = 0
		Select align
			Case ALIGN_CENTER
				DrawHTML(txt,x-(TextWidth(StripHTML(txt))/2),y)
			Case ALIGN_RIGHT
				DrawHTML(txt,x-TextWidth(StripHTML(txt)),y)
			Case ALIGN_LEFT
				DrawHTML(txt,x,y)
		End Select
	End Method
	
	Function StripHTML:String(txt:String)
		Local plainText:String = txt.Replace("</","<")
		plainText = plainText.Replace("<b>","")
		Return plainText.Replace("<i>","")
	End

	Method TextWidth:Int(txt:String)
'		Local prevChar:String = ""
		Local prevChar:Int = 0
		Local width:Int = 0
		For Local i:= 0 Until txt.Length
			Local asc:Int = txt[i]
			Local ac:Char = chars[asc]
'			Local thisChar:String = String.FromChar(asc)
			Local thisChar:Int = asc
			If ac  <> Null
				If useKerning
					Local firstKp:= kernPairs.Get(prevChar)
					If firstKp <> Null
						Local secondKp:= firstKp.Get(thisChar)
						If secondKp <> Null
							xOffset += secondKp.amount
						End							
					Endif
				Endif
				'ch.Draw(image, x+xOffset,y)
				width += ac.xAdvance
				prevChar = thisChar
			Endif
		Next
		Return width
	End Method
	
	Method TextHeight:Int(txt:String)
		Local h:Int = 0
		For Local i:= 0 Until txt.Length
			Local asc:Int = txt[i]
			Local ac:Char = chars[asc]
			If ac.height+ac.yOffset > h h = ac.height+ac.yOffset
		Next
		Return h
	End

End Class
