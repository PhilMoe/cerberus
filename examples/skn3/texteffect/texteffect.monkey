
Import mojo

Const CHARACTER_ASCII := 0
Const CHARACTER_WIDTH := 1
Const CHARACTER_HEIGHT := 2
Const CHARACTER_PAGE := 3
Const CHARACTER_RECT_X := 4
Const CHARACTER_RECT_Y := 5
Const CHARACTER_RECT_WIDTH := 6
Const CHARACTER_RECT_HEIGHT := 7
Const CHARACTER_OFFSET_X := 8
Const CHARACTER_OFFSET_Y := 9

Function Main()
    New myApp
End

Class myApp Extends App
    Field font:Font
    Field message:String = "Welcome to the text thingy written in Monkey. This is a long piece of text but it should wrap onto new lines. The wrap effect does not have a popin problem as the text is not calculated character by character, but isntead word by word!"
    Field messageCount:Int = 0
    Field messageLast:Int = 0
    Field messageSpeed:Int = 50

    Method OnCreate()
        font = New Font(22,27,["font.png"],[[32,5,27,0,0,0,0,0,0,19],[33,5,27,0,393,46,7,18,-1,4],[34,9,27,0,122,99,10,10,-1,3],[35,15,27,0,51,66,17,17,-1,5],[36,16,27,0,478,0,18,22,-1,2],[37,20,27,0,332,25,21,21,-1,2],[38,16,27,0,34,66,17,17,-1,5],[39,5,27,0,116,99,6,10,-1,3],[40,9,27,0,89,0,11,24,-1,2],[41,9,27,0,78,0,11,24,-1,2],[42,14,27,0,378,46,15,18,-1,4],[43,12,27,0,386,83,14,14,-1,6],[44,6,27,0,109,99,7,10,-1,14],[45,10,27,0,174,99,12,7,-1,10],[46,5,27,0,219,99,6,6,-1,15],[47,12,27,0,366,25,13,21,-1,2],[48,18,27,0,24,83,19,16,-1,5],[49,12,27,0,60,83,14,16,-1,5],[50,14,27,0,171,83,15,15,-1,6],[51,16,27,0,154,83,17,15,-1,6],[52,15,27,0,154,66,17,17,-1,5],[53,15,27,0,138,66,16,17,-1,5],[54,16,27,0,121,66,17,17,-1,5],[55,13,27,0,106,66,15,17,-1,5],[56,15,27,0,89,66,17,17,-1,5],[57,15,27,0,43,83,17,16,-1,6],[58,5,27,0,424,83,6,14,-1,7],[59,6,27,0,68,66,7,17,-1,7],[60,10,27,0,412,83,12,14,-1,7],[61,14,27,0,79,99,16,11,-1,7],[62,10,27,0,400,83,12,14,-1,7],[63,13,27,0,75,66,14,17,-1,5],[64,21,27,0,14,46,22,20,-1,6],[65,17,27,0,461,66,18,17,-1,5],[66,14,27,0,446,66,15,17,-1,5],[67,15,27,0,430,66,16,17,-1,5],[68,15,27,0,414,66,16,17,-1,5],[69,14,27,0,399,66,15,17,-1,5],[70,13,27,0,385,66,14,17,-1,5],[71,15,27,0,369,66,16,17,-1,5],[72,14,27,0,353,66,16,17,-1,5],[73,11,27,0,142,83,12,16,-1,6],[74,15,27,0,126,83,16,16,-1,5],[75,14,27,0,337,66,16,17,-1,5],[76,14,27,0,322,66,15,17,-1,5],[77,17,27,0,304,66,18,17,-1,5],[78,15,27,0,287,66,17,17,-1,5],[79,18,27,0,107,83,19,16,-1,5],[80,15,27,0,416,46,16,18,-1,4],[81,18,27,0,268,66,19,17,-1,5],[82,15,27,0,400,46,16,18,-1,4],[83,16,27,0,250,66,18,17,-1,5],[84,15,27,0,319,83,16,15,-1,6],[85,15,27,0,90,83,17,16,-1,5],[86,16,27,0,233,66,17,17,-1,5],[87,16,27,0,215,66,18,17,-1,5],[88,15,27,0,199,66,16,17,-1,5],[89,14,27,0,184,66,15,17,-1,5],[90,15,27,0,74,83,16,16,-1,5],[91,9,27,0,67,0,11,24,-1,2],[92,12,27,0,353,25,13,21,-1,2],[93,9,27,0,56,0,11,24,-1,2],[94,12,27,0,26,99,13,12,-1,2],[95,15,27,0,203,99,16,6,-1,20],[96,7,27,0,141,99,8,9,-1,3],[97,14,27,0,304,83,15,15,-1,7],[98,12,27,0,287,46,13,19,-1,3],[99,12,27,0,472,83,13,14,-1,7],[100,12,27,0,70,46,14,20,-1,2],[101,13,27,0,290,83,14,15,-1,7],[102,12,27,0,274,46,13,19,-1,3],[103,13,27,0,260,46,14,19,-1,7],[104,13,27,0,246,46,14,19,-1,3],[105,5,27,0,239,46,7,19,-1,3],[106,5,27,0,315,0,7,23,-1,3],[107,12,27,0,56,46,14,20,-1,2],[108,5,27,0,50,46,6,20,-1,2],[109,16,27,0,272,83,18,15,-1,7],[110,13,27,0,258,83,14,15,-1,7],[111,12,27,0,458,83,14,14,-1,7],[112,12,27,0,36,46,14,20,-1,7],[113,13,27,0,225,46,14,19,-1,7],[114,10,27,0,246,83,12,15,-1,7],[115,13,27,0,232,83,14,15,-1,7],[116,12,27,0,171,66,13,17,-1,4],[117,12,27,0,218,83,14,15,-1,7],[118,13,27,0,443,83,15,14,-1,7],[119,16,27,0,200,83,18,15,-1,7],[120,13,27,0,186,83,14,15,-1,7],[121,13,27,0,211,46,14,19,-1,7],[122,12,27,0,430,83,13,14,-1,8],[123,12,27,0,28,0,14,25,-1,2],[124,5,27,0,309,0,6,23,-1,3],[125,12,27,0,14,0,14,25,-1,2],[126,15,27,0,157,99,17,8,-1,10],[127,13,27,0,0,46,14,20,-1,3],[161,5,27,0,371,46,7,18,-1,9],[162,12,27,0,465,0,13,22,-1,3],[163,14,27,0,18,66,16,17,-1,5],[164,16,27,0,0,66,18,17,-1,4],[165,14,27,0,481,46,15,17,-1,5],[166,5,27,0,303,0,6,23,-1,3],[167,13,27,0,317,25,15,21,-1,5],[168,9,27,0,192,99,11,6,-1,5],[169,21,27,0,189,46,22,19,-1,5],[170,13,27,0,65,99,14,11,-1,5],[171,17,27,0,368,83,18,14,-1,7],[172,12,27,0,95,99,14,10,-1,10],[174,21,27,0,167,46,22,19,-1,5],[176,12,27,0,52,99,13,11,-1,4],[177,13,27,0,357,46,14,18,-1,4],[178,12,27,0,0,99,13,13,-1,4],[179,12,27,0,13,99,13,12,-1,5],[180,8,27,0,132,99,9,9,-1,3],[181,14,27,0,152,46,15,19,-1,7],[182,18,27,0,297,25,20,21,-1,4],[183,5,27,0,186,99,6,6,-1,10],[184,7,27,0,149,99,8,8,-1,17],[185,7,27,0,498,83,8,13,-1,5],[186,11,27,0,39,99,13,11,-1,5],[187,17,27,0,350,83,18,14,-1,7],[188,17,27,0,278,25,19,21,-1,2],[189,18,27,0,259,25,19,21,-1,2],[190,18,27,0,239,25,20,21,-1,2],[191,13,27,0,467,46,14,17,-1,10],[192,17,27,0,285,0,18,23,-1,-1],[193,17,27,0,267,0,18,23,-1,-1],[194,17,27,0,249,0,18,23,-1,-1],[195,17,27,0,447,0,18,22,-1,0],[196,17,27,0,221,25,18,21,-1,1],[197,17,27,0,231,0,18,23,-1,-1],[198,22,27,0,0,83,24,16,-1,6],[199,15,27,0,486,25,16,20,-1,5],[200,14,27,0,216,0,15,23,-1,-1],[201,14,27,0,201,0,15,23,-1,-1],[202,14,27,0,186,0,15,23,-1,-1],[203,14,27,0,206,25,15,21,-1,1],[204,11,27,0,174,0,12,23,-1,-1],[205,11,27,0,162,0,12,23,-1,-1],[206,12,27,0,148,0,14,23,-1,-1],[207,11,27,0,194,25,12,21,-1,1],[208,17,27,0,448,46,19,17,-1,5],[209,15,27,0,430,0,17,22,-1,0],[210,18,27,0,411,0,19,22,-1,-1],[211,18,27,0,392,0,19,22,-1,-1],[212,18,27,0,373,0,19,22,-1,-1],[213,18,27,0,175,25,19,21,-1,0],[214,18,27,0,467,25,19,20,-1,1],[215,12,27,0,485,83,13,13,-1,7],[216,18,27,0,129,0,19,23,-1,2],[217,15,27,0,356,0,17,22,-1,-1],[218,15,27,0,339,0,17,22,-1,-1],[219,15,27,0,322,0,17,22,-1,-1],[220,15,27,0,450,25,17,20,-1,1],[221,14,27,0,114,0,15,23,-1,-1],[222,14,27,0,432,46,16,17,-1,5],[223,14,27,0,342,46,15,18,-1,4],[224,14,27,0,160,25,15,21,-1,1],[225,14,27,0,145,25,15,21,-1,1],[226,14,27,0,130,25,15,21,-1,1],[227,14,27,0,435,25,15,20,-1,2],[228,14,27,0,137,46,15,19,-1,3],[229,14,27,0,115,25,15,21,-1,1],[230,21,27,0,479,66,23,16,-1,6],[231,12,27,0,329,46,13,18,-1,7],[232,13,27,0,101,25,14,21,-1,1],[233,13,27,0,87,25,14,21,-1,1],[234,13,27,0,72,25,15,21,-1,1],[235,13,27,0,123,46,14,19,-1,3],[236,7,27,0,64,25,8,21,-1,1],[237,8,27,0,55,25,9,21,-1,1],[238,11,27,0,42,25,13,21,-1,1],[239,9,27,0,112,46,11,19,-1,3],[240,14,27,0,314,46,15,18,-1,3],[241,13,27,0,421,25,14,20,-1,2],[242,12,27,0,407,25,14,20,-1,1],[243,12,27,0,393,25,14,20,-1,1],[244,12,27,0,379,25,14,20,-1,1],[245,12,27,0,98,46,14,19,-1,2],[246,12,27,0,300,46,14,18,-1,3],[247,14,27,0,335,83,15,14,-1,5],[248,12,27,0,28,25,14,21,-1,4],[249,12,27,0,14,25,14,21,-1,1],[250,12,27,0,0,25,14,21,-1,1],[251,12,27,0,496,0,14,21,-1,1],[252,12,27,0,84,46,14,19,-1,3],[253,13,27,0,0,0,14,25,-1,1],[254,12,27,0,42,0,14,24,-1,3],[255,13,27,0,100,0,14,23,-1,3]])
        messageLast = Millisecs()
        SetUpdateRate(60)
    End

    Method OnUpdate()
		If Millisecs() - messageLast >= messageSpeed
            messageCount += 1
            messageLast = Millisecs()
		Endif
    End

    Method OnRender()
		SetColor(255,255,255)
		DrawRect(5,5,200,470)
        font.Wrap(message,5,5,200,470,messageCount)
    End
End

Class Font
    Field width:Int = 0
    Field height:Int = 0
    Field pages:Image[0]
    Field baseCharacter:Int = 0
    Field characters:FontCharacter[256]

    Method New(_width:Int,_height:Int,_pages:String[],_characters:Int[][])
        ' --- setup the font object with the data passed in! ---
        width = _width
        height = _height

        'load in the page images
        pages = New Image[_pages.Length()]
        For Local index := 0 Until _pages.Length()
            pages[index] = LoadImage(_pages[index])
        Next

        'create the
        For Local character := Eachin _characters
            characters[character[CHARACTER_ASCII]] = New FontCharacter(character[CHARACTER_WIDTH],character[CHARACTER_HEIGHT],pages[character[CHARACTER_PAGE]],character[CHARACTER_RECT_X],character[CHARACTER_RECT_Y],character[CHARACTER_RECT_WIDTH],character[CHARACTER_RECT_HEIGHT],character[CHARACTER_OFFSET_X],character[CHARACTER_OFFSET_Y])
        Next
    End

    Method Draw(text:String,x:Float,y:Float)
        ' --- draw a single line of text ---
        Local length:Int = text.Length()
        Local character:FontCharacter

        For Local index := 0 Until length
            character = characters[text[index]]
            If character <> Null
                print text[index]
                DrawImage(character.image,x,y)
                x += character.width
            Endif
        Next
    End

    Method Wrap(text:String,containerX:Float,containerY:Float,containerWidth:Float,containerHeight:Float,count:Int=-1)
        ' --- wrap text within an area ---
        'get the number of characters that will be rendered from the text
        If count < 0 Or count > text.Length() count = text.Length()

        Local drawX:Float = containerX
        Local drawY:Float = containerY
        Local lineStart:Bool = True
        Local textIndex:Int = 0
        Local textLength = text.Length()
        Local wordStart:Int
        Local wordLength:Int
        Local wordWidth:Int
        Local character:FontCharacter

        While textIndex < count
            'find the next complete word!
            wordStart = textIndex
            wordLength = 0
            For Local wordIndex := textIndex Until textLength
                If text[wordIndex] = 32
                    'break out as a word has been found
                    Exit
                Else
                    'increase the word length
                    wordLength += 1
                Endif
            Next

            'calculate the width of the word
            wordWidth = 0
            For Local wordIndex := wordStart Until wordStart + wordLength
                character = characters[text[wordIndex]]
                If character <> Null wordWidth += character.width
            Next

            'add split character
            If lineStart = False
                character = characters[32]
                If character
                    If drawX + character.width <= containerX + containerWidth
                        DrawImage(character.image,drawX,drawY)
                        drawX += character.width
                    Else
                        drawX = containerX
                        drawY += height
                        If drawY + height > containerHeight Return
                        lineStart = True
                    Endif
                Endif
            Endif

            'make sure only drawing upto count!
            textIndex += 1
            If textIndex > count Return

            'render the word
            'print text[wordStart..wordStart+wordLength]+" :: "+(drawX + wordWidth)+" / "+(containerX + containerWidth)
            If drawX + wordWidth <= containerX + containerWidth Or wordWidth <= containerWidth
                'move WHOLE word onto new line
                If drawX + wordWidth > containerX + containerWidth
                    drawX = containerX
                    drawY += height
                    If drawY + height > containerHeight Return
                    lineStart = True
                Endif

                'draw word as a whole!
                For Local drawIndex := wordStart Until wordStart + wordLength
                    'print String.FromChar(text[drawIndex])
                    character = characters[text[drawIndex]]
                    If character <> Null
                        DrawImage(character.image,drawX,drawY)
                        drawX += character.width
                    Endif

                    'make sure only drawing upto count!
                    textIndex += 1
                    If textIndex > count Return
                Next
                lineStart = False
            Else
                'need to split the word onto multiple lines!
                 For Local drawIndex := wordStart Until wordStart + wordLength
                    character = characters[text[drawIndex]]
                    If character
                        'look for going onto new line!
                        If drawX + character.width > containerX + containerWidth
                            drawX = containerX
                            drawY += height
                            If drawY + height > containerHeight Return
                            lineStart = True
                        Endif

                        'draw the character
                        DrawImage(character.image,drawX,drawY)
                        drawX += character.width
                        lineStart = False
                    Endif

                    'make sure only drawing upto count!
                    textIndex += 1
                    If textIndex > count Return
                Next
            Endif
        Wend
    End
End

Class FontCharacter
    Field width:Int
    Field height:Int
    Field image:Image

    Method New(_width:Int,_height:Int,page:Image,rectX:Int,rectY:Int,rectWidth:Int,rectHeight:Int,offsetX:Int,offsetY:Int)
        width = _width
        height = _height
        image = page.GrabImage(rectX,rectY,rectWidth,rectHeight)
        image.SetHandle(-offsetX,-offsetY)
    End
End

