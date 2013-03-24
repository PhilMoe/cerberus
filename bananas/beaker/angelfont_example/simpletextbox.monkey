Strict

'Import mojo
Import angelfont

Class SimpleTextBox
	Global lineGap:Int = 5
	
	Global yOffset:Int = 0
	
	Global font:AngelFont
	
	Global align:Int = AngelFont.ALIGN_LEFT
	
	Function Draw:Void(text:String, x:Int, y:Int, width:Int, alignment:Int = AngelFont.ALIGN_LEFT)	
		
		Local thisLine:String = ""
		Local charOffset:Int = 0
		
		Local wordLen:Int = 0
		Local word:String = ""
		
		font = AngelFont.current
		align = alignment
		
		yOffset = 0
		For Local i := 0 Until text.Length
			If y+yOffset > DeviceHeight()
				Return
			Endif		
		
			Local asc:Int = text[i]
			Select asc
				Case 32	' space
					wordLen = font.TextWidth(word)
					If charOffset + wordLen > width
						DrawTextLine(thisLine, x,y+yOffset)
						thisLine = ""
						charOffset = 0
					Endif
'					Local chars:Char[] = font.GetChars()
					charOffset += wordLen+font.GetChars()[32].xAdvance
					thisLine += word + " "
					
					word = ""
					'wordLen = 0
				Case 10	' newline or "~n"
					wordLen = font.TextWidth(word)
					If charOffset + wordLen > width
						DrawTextLine(thisLine, x,y+yOffset)
						thisLine = ""
					Endif
					thisLine += word
				
					DrawTextLine(thisLine, x,y+yOffset)

					thisLine = ""
					charOffset = 0
					word = ""
				Default
					'Local ch:Char = font.GetChars()[asc]
					word += String.FromChar(asc)
			End Select
		Next

		If word <> ""
			wordLen = font.TextWidth(word)
			If charOffset + wordLen > width
				DrawTextLine(thisLine, x,y+yOffset)
				thisLine = ""
			Endif
			thisLine += word
		Endif
		If thisLine <> ""
			DrawTextLine(thisLine, x,y+yOffset)
		Endif
	End
	
	Function DrawHTML:Void(text:String, x:Int, y:Int, width:Int, alignment:Int = AngelFont.ALIGN_LEFT)	
		Local thisLine:String = ""
		Local charOffset:Int = 0
		
		Local wordLen:Int = 0
		Local word:String = ""
		
		font = AngelFont.current
		align = alignment
		
		yOffset = 0
		For Local i := 0 Until text.Length
			If y+yOffset > DeviceHeight()
				Return
			Endif		
		
			Local asc:Int = text[i]
			Select asc
				Case 32	' space
					wordLen = font.TextWidth(AngelFont.StripHTML(word))
					If charOffset + wordLen > width
						DrawTextLineHTML(thisLine, x,y+yOffset)
						thisLine = ""
						charOffset = 0
					Endif
'					Local chars:Char[] = font.GetChars()
					charOffset += wordLen+font.GetChars()[32].xAdvance
					thisLine += word + " "
					
					word = ""
					'wordLen = 0
				Case 10	' newline or "~n"
					wordLen = font.TextWidth(AngelFont.StripHTML(word))
					If charOffset + wordLen > width
						DrawTextLineHTML(thisLine, x,y+yOffset)
						thisLine = ""
					Endif
					thisLine += word
				
					DrawTextLineHTML(thisLine, x,y+yOffset)

					thisLine = ""
					charOffset = 0
					word = ""
				Case 60	' <
					If text[i+1..i+3] = "i>" Or text[i+1..i+3] = "b>"
						word += text[i..i+3]
						i += 2
					Else
						If text[i+1..i+4] = "/i>" Or text[i+1..i+4] = "/b>"
							word += text[i..i+4]
							i += 3
						End
					End
				Default
					'Local ch:Char = font.GetChars()[asc]
					word += String.FromChar(asc)
			End Select
		Next

		If word <> ""
			wordLen = font.TextWidth(AngelFont.StripHTML(word))
			If charOffset + wordLen > width
				DrawTextLineHTML(thisLine, x,y+yOffset)
				thisLine = ""
			Endif
			thisLine += word
		Endif
		If thisLine <> ""
			DrawTextLineHTML(thisLine, x,y+yOffset)
		Endif
	End
	
	Function DrawTextLine:Void(txt:String, x:Int,y:Int)
		font.DrawText(txt, x,y, align)
		yOffset += lineGap+font.height
	End
	
	Function DrawTextLineHTML:Void(txt:String, x:Int,y:Int)
		font.DrawHTML(txt, x,y, align)
		yOffset += lineGap+font.height
	End


End Class


















