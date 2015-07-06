
#GLFW_WINDOW_WIDTH=900
#GLFW_WINDOW_HEIGHT=480

Import mojo
Import brl.httprequest
Import monkey

'main program
Function Main:Int()
	New IconFun()
End

'http getter module code
Interface HTTPListener

	Method OnHTTPConnected:Void( getter:HTTPGetter )

	Method OnHTTPDataReceived:Void( data:DataBuffer,offset:Int,count:Int,getter:HTTPGetter )

	Method OnHTTPPageComplete:Void( getter:HTTPGetter )

End

Class HTTPGetter Implements IOnReadComplete,IOnWriteComplete,IOnConnectComplete
	Field path:String
	
	Method GetPage:Void(host:String, port:Int, listener:HTTPListener)
	
		'need to extract the path if there is one
		Local pos:Int = host.Find("/")
		If pos = -1
			_host = host
			path = ""
		Else
			_host = host[0 .. pos]
			path = host[pos ..]
		Endif

		_port = port
		_listener=listener
		
		_stream=New AsyncTcpStream
		
		_stream.Connect _host,_port,Self
	End
	
	Private
	
	Method Finish:Void()
		_listener.OnHTTPPageComplete Self
		_strqueue.Clear
		_stream.Close
		_stream=Null
	
	End
	
	'start up another read op
	Method ReadMore:Void()
		'read another block
		_stream.ReadAll _rbuf,0,_rbuf.Length,Self
	End

	'start up another write op
	Method WriteMore:Void()
	
		If _strqueue.IsEmpty() Return
		
		Local str:=_strqueue.RemoveFirst()
		
		_wbuf.PokeString 0,str
		
		_stream.WriteAll _wbuf,0,str.Length,Self
	End
	
	Method WriteString:Void( str:String )
	
		_strqueue.AddLast str
		
	End

	Method OnConnectComplete:Void( connected:Bool,source:IAsyncEventSource )
	
		If Not connected
			Finish
			Return
		Endif

		If path.Length = 0
			WriteString "GET / HTTP/1.0~r~n"
		Else
			WriteString "GET http://" + _host + ":" + _port + path + " HTTP/1.0~r~n"
		Endif
		
		WriteString "Host: " + _host + "~r~n"
		WriteString "~r~n"

		_listener.OnHTTPConnected Self

		WriteMore
		
		ReadMore
	End

	Method OnReadComplete:Void( buf:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
	
		If Not count	'EOF!
			Finish
			Return
		Endif
		
		_listener.OnHTTPDataReceived buf,offset,count,Self
		
		ReadMore
	End
	
	Method OnWriteComplete:Void( buf:DataBuffer,offset:Int,count:Int,source:IAsyncEventSource )
	
		WriteMore
	End
	
	Field _host:String
	Field _port:Int
	Field _listener:HTTPListener
	
	Field _stream:AsyncTcpStream
	Field _strqueue:=New StringList
	Field _rbuf:= New DataBuffer(256)	'thrash it!
	Field _wbuf:= New DataBuffer(2048)
End

'main app
Class IconFun Extends App Implements IOnHttpRequestComplete
	Const ICON_SIZE:Int = 32
	
	Const MODE_IDLE:Int = 0
	Const MODE_DRAW:Int = 1
	Const MODE_REMOVE:Int = 2
	Const MODE_LOAD:Int = 3
	Const MODE_SAVE:Int = 4
	
	Const REMOTE_SERVER:String = "www.skn3.com:80/junk/iconfun/index.php"
	
	Field paletteR:Int[9]
	Field paletteG:Int[9]
	Field paletteB:Int[9]
	
	Field editorDisabled:Int = false
	Field editorPalette:Int = 1
	Field editorPaletteSize:Int = (ICON_SIZE * 8) / 9
	Field editorPixels:Int[ICON_SIZE * ICON_SIZE]
	Field editorPixelSize:Int = 8
	Field editorRootX:Int
	Field editorRootY:int
	Field editorSize:Int = ICON_SIZE * 8
	Field editorPixelX:Int
	Field editorPixelY:Int
	Field editorTitleInput:TextInput
	Field editorAuthorInput:TextInput
	Field editorSubmitInput:ButtonInput
	Field editorSpacing:Int = 5
	Field editorGadgetHeight:Int = 24
	Field editorTransparent:= False
	
	Global ms:Int
	Global fontWhite:Image
	Global fontBlack:Image
	
	Field icons:= new List<Icon>
	Field iconsPending:= new List<Icon>
	
	Field http:HttpRequest
	Field httpMode:Int = MODE_IDLE
	
	Field loadTimestamp:Int
	Field loadInterval:Int = 20000
	Field loadNext:Int = true
	Field loadRemoteTimestamp:String
	
	Field saveSuccess:Int = false
	Field saveNext:Int = false
	
	Field busyImage:Image
	Field busyFrame:Int = 0
	Field busyFrameTimestamp:Int
	Field busyEndTimestamp:Int
	Field busyText:string
	
	Method OnCreate:Int()
		' --- setup the app ---
		'setup editor root position
		editorRootX = DeviceWidth() -276
		editorRootY = 20
		
		'create palette colors
		'transparent
		paletteR[0] = -1
		paletteG[0] = -1
		paletteB[0] = -1
		
		'black
		paletteR[1] = 0
		paletteG[1] = 0
		paletteB[1] = 0
		
		'white
		paletteR[2] = 255
		paletteG[2] = 255
		paletteB[2] = 255
		
		'blue
		paletteR[3] = 60
		paletteG[3] = 120
		paletteB[3] = 206
		
		'red
		paletteR[4] = 189
		paletteG[4] = 63
		paletteB[4] = 51
		
		'green
		paletteR[5] = 85
		paletteG[5] = 154
		paletteB[5] = 60
		
		'yellow
		paletteR[6] = 245
		paletteG[6] = 215
		paletteB[6] = 73
		
		'purple
		paletteR[7] = 131
		paletteG[7] = 82
		paletteB[7] = 168
		
		'grey
		paletteR[8] = 133
		paletteG[8] = 133
		paletteB[8] = 133
		
		'setup app
		SetUpdateRate(60)
		
		'setup the gui elements
		Local guiX:Int = editorRootX
		Local guiY:Int = editorRootY + editorSize + editorSpacing + editorPaletteSize + editorSpacing
		
		'title
		editorTitleInput = New TextInput("Title", guiX, guiY, editorSize, editorGadgetHeight, 26)
		guiY = guiY + editorSpacing + editorGadgetHeight
		
		'author
		editorAuthorInput = New TextInput("Author", guiX, guiY, editorSize, editorGadgetHeight, 18)
		guiY = guiY + editorSpacing + editorGadgetHeight
		
		'button
		editorSubmitInput = New ButtonInput("Submit", guiX, guiY, editorSize, editorGadgetHeight)
		guiY = guiY + editorSpacing + editorGadgetHeight
		
		'load image resources
		busyImage = LoadImage("loading.png", 6, Image.MidHandle)
		fontWhite = LoadImage("font_white.png", 96, Image.XPadding)
		fontBlack = LoadImage("font_black.png", 96, Image.XPadding)
	End
	
	Method OnUpdate:int()
		' --- update ---
		Local index:Int
		Local pointerX:Int = MouseX()
		Local pointerY:Int = MouseY()
		Local pointerMode:Int
		Local pointerHit:Int
		Local editorX:Float
		Local editorY:Float
		
		'save millisecs
		ms = Millisecs()
		
		'update http first so we might trigger more http this update
		UpdateAsyncEvents
		
		'process save (this must be first)
		if httpMode = MODE_IDLE And saveNext
			saveNext = False
			SaveIcon()
		EndIf
		
		'process load
		if httpMode = MODE_IDLE And (ms > loadTimestamp + loadInterval or loadNext)
			loadNext = False
			LoadIcons()
		EndIf
		
		'process pending icons
		if iconsPending.IsEmpty() = False
			'just do one per frame
			Local icon:= iconsPending.RemoveFirst()
			Local pixels:Int[ICON_SIZE * ICON_SIZE]
			Local paletteIndex:Int
			
			'create the image
			icon.image = CreateImage(ICON_SIZE, ICON_SIZE, 1, Image.MidHandle)
			
			'go through the data
			For index = 0 Until icon.data.Length
				Local i:=icon.data[index];

				If i<48 Or i>48+8
					Print "ERROR! i="+i+", index="+index
				Endif

				paletteIndex = Min(8, Max(0, Int(String.FromChar(icon.data[index]))))
				
				if paletteIndex = 0
					'transparent
					pixels[index] = ARGB(255, 0, 255, 0)
				Else
					'solid
					pixels[index] = ARGB(paletteR[paletteIndex], paletteG[paletteIndex], paletteB[paletteIndex])
				EndIf
			Next
			
			'write to the image
			icon.image.WritePixels(pixels, 0, 0, ICON_SIZE, ICON_SIZE)
			
			'add to loaded icons
			icons.AddLast(icon)
		EndIf
		
		'update icons
		For Local icon:= eachin icons
			if icon.appear > 0
				icon.appear = icon.appear - 0.02
				if icon.appear < 0 icon.appear = 0
				icon.scale = 1.0 + (4.0 * icon.appear)
				icon.angle = 0 + (-45 * icon.appear)
			EndIf
			
			if icon.appear = 0
				if icon.vx > 0
					if icon.x + icon.vx > DeviceWidth() - (ICON_SIZE / 2)
						icon.x = DeviceWidth() - (ICON_SIZE / 2)
						icon.vx = -icon.vx
					Else
						icon.x += icon.vx
					EndIf
				ElseIf icon.vx < 0
					if icon.x + icon.vx < (ICON_SIZE / 2)
						icon.x = (ICON_SIZE / 2)
						icon.vx = -icon.vx
					Else
						icon.x += icon.vx
					EndIf
				EndIf
				
				if icon.vy > 0
					if icon.y + icon.vy > DeviceHeight() - (ICON_SIZE / 2)
						icon.y = DeviceHeight() - (ICON_SIZE / 2)
						icon.vy = -icon.vy
					Else
						icon.y += icon.vy
					EndIf
				ElseIf icon.vy < 0
					if icon.y + icon.vy < (ICON_SIZE / 2)
						icon.y = (ICON_SIZE / 2)
						icon.vy = -icon.vy
					Else
						icon.y += icon.vy
					EndIf
				EndIf
			EndIf
		Next
		
		'get mouse states
		if MouseHit(MOUSE_LEFT)
			pointerMode = MODE_DRAW
			pointerHit = true
		ElseIf MouseHit(MOUSE_RIGHT)
			pointerMode = MODE_REMOVE
			pointerHit = true
		elseif MouseDown(MOUSE_LEFT)
			pointerMode = MODE_DRAW
			pointerHit = false
		ElseIf MouseDown(MOUSE_RIGHT)
			pointerMode = MODE_REMOVE
			pointerHit = false
		EndIf
		
		'check for mouse interaction with editor pixels
		If editorDisabled = False
			'check if in editro box
			If pointerX >= editorRootX - editorSpacing And pointerY >= editorRootY - editorSpacing And pointerX <= editorRootX - editorSpacing + editorSize + editorSpacing + editorSpacing And pointerY <= editorRootY - editorSpacing + editorSize + editorSpacing + editorPaletteSize + editorSpacing + editorSpacing + editorGadgetHeight + editorSpacing + editorGadgetHeight + editorSpacing + editorGadgetHeight + editorSpacing
				editorTransparent = False
			Else
				editorTransparent = True
			EndIf
		
			editorX = pointerX - editorRootX
			editorY = pointerY - editorRootY
			If editorX >= 0 And editorY >= 0 And editorX < editorSize And editorY < editorSize
				'pointer is within editor
				editorPixelX = (editorX / editorPixelSize)
				editorPixelY = (editorY / editorPixelSize)
				
				'work out the pixel index we are on
				index = (ICON_SIZE * editorPixelY) + editorPixelX
				
				Select pointerMode
					Case MODE_DRAW
						editorPixels[index] = editorPalette
						TextInput.AllInactive()
					Case MODE_REMOVE
						editorPixels[index] = 0
						TextInput.AllInactive()
				End
			EndIf
			
			'check for mouse interaction with editor palette
			editorX = pointerX - editorRootX
			editorY = pointerY - editorRootY - editorSpacing - editorSize
			if pointerHit and editorX >= 0 And editorY >= 0 And editorX < editorSize And editorY < editorPaletteSize editorPalette = Min(8, int(editorX / editorPaletteSize))
			
			'update button state
			if editorTitleInput.HasValue() = false or editorAuthorInput.HasValue() = false
				editorSubmitInput.active = false
			Else
				editorSubmitInput.active = true
			EndIf
			
			'update gui
			TextInput.AllUpdate()
			ButtonInput.AllUpdate()
			
			'CHGUI_Update()
			If editorSubmitInput.pressed Then SaveIcon()
		EndIf
		
		'update busy icon
		if httpMode <> MODE_IDLE or ms < busyEndTimestamp + 500
			if ms > busyFrameTimestamp + 30
				busyFrameTimestamp = ms
				busyFrame += 1
				if busyFrame >= busyImage.Frames busyFrame = 0
			EndIf
		Else
			if editorDisabled editorDisabled = false
		EndIf
	End
	
	Method OnRender:int()
		' --- render ---
		Local y:int
		Local x:Int
		Local index:Int
		Local drawX:Int
		Local drawY:Int
		Local drawWidth:Int
		
		'setup canvas
		Cls(0, 0, 0)
		SetFont(fontWhite)
		
		'render icons
		PushMatrix()
		For Local icon:= eachin icons
			drawX = icon.x
			drawY = icon.y
			
			if icon.angle = 0 DrawText(icon.title + " (" + icon.author + ")", drawX - int(TextWidth(icon.title + " (" + icon.author + ")") / 2), drawY - (ICON_SIZE / 2) - FontHeight() -2)
			DrawImage(icon.image, drawX, drawY, icon.angle, icon.scale, icon.scale, 0)
		Next
		
		'render editor
		If editorDisabled Or editorTransparent
			SetAlpha(0.2)
		Else
			SetAlpha(1.0)
		EndIf
		
		'editor background
		SetColor(99, 99, 99)
		DrawRect(editorRootX - editorSpacing, editorRootY - editorSpacing, editorSize + editorSpacing + editorSpacing, editorSize + editorSpacing + editorPaletteSize + editorSpacing + editorSpacing + editorGadgetHeight + editorSpacing + editorGadgetHeight + editorSpacing + editorGadgetHeight + editorSpacing)
		
		'render editor pixels
		'transparent pixels first
		For y = 0 Until ICON_SIZE
			For x = 0 Until ICON_SIZE
				index = editorPixels[ (y * ICON_SIZE) + x]
				drawX = editorRootX + (x * editorPixelSize)
				drawY = editorRootY + (y * editorPixelSize)
				
				if index = 0
					'transparent pixel
					SetColor(64, 64, 64)
					DrawLineRect(drawX, drawY, editorPixelSize, editorPixelSize)
				EndIf
			Next
		Next
		
		'color pixels second
		For y = 0 Until ICON_SIZE
			For x = 0 Until ICON_SIZE
				index = editorPixels[ (y * ICON_SIZE) + x]
				drawX = editorRootX + (x * editorPixelSize)
				drawY = editorRootY + (y * editorPixelSize)
				
				if index > 0
					'color pixel
					SetColor(paletteR[index], paletteG[index], paletteB[index])
					DrawRect(drawX, drawY, editorPixelSize, editorPixelSize)
				EndIf
			Next
		Next
		
		'render palette
		drawY = editorRootY + editorSize + editorSpacing
		For index = 0 Until paletteR.Length
			drawX = editorRootX + (index * editorPaletteSize)
			
			if index = paletteR.Length - 1
				'last item
				drawWidth = editorSize - (drawX - editorRootX)
			Else
				'not last item
				drawWidth = editorPaletteSize
			EndIf
			
			if index = 0
				SetColor(0, 0, 0)
				DrawLineRect(drawX, drawY, drawWidth, editorPaletteSize)
				DrawLine(drawX, drawY, drawX + drawWidth, drawY + editorPaletteSize)
				DrawLine(drawX + drawWidth, drawY, drawX, drawY + editorPaletteSize)
			Else
				SetColor(paletteR[index], paletteG[index], paletteB[index])
				DrawRect(drawX, drawY, drawWidth, editorPaletteSize)
			EndIf
			
			'focus rect
			if editorPalette = index
				SetColor(0, 255, 0)
				DrawLineRect(drawX, drawY, drawWidth, editorPaletteSize)
				DrawLineRect(drawX + 1, drawY + 1, drawWidth - 2, editorPaletteSize - 2)
			EndIf
		Next
		
		'render editor outline
		SetColor(255, 255, 255)
		DrawLineRect(editorRootX, editorRootY, editorSize, editorSize)
		
		'render gui
		TextInput.AllRender()
		ButtonInput.AllRender()
		
		'render loading
		if httpMode <> MODE_IDLE or ms < busyEndTimestamp + 1000
			SetColor(0, 0, 0)
			DrawRect(3, 3, DeviceWidth() -6, 20)
			SetColor(0, 255, 0)
			DrawLineRect(3, 3, DeviceWidth() -6, 20)
			SetColor(255, 255, 255)
			DrawImage(busyImage, 13, 13, busyFrame)
			SetFont(fontWhite)
			DrawText(busyText, 23, 7)
		EndIf
	End
	
	Method OnHttpRequestComplete:Void(req:HttpRequest)
		' --- http request completed ---
		Select httpMode
			Case MODE_SAVE
				'add data to http buffer
				Local httpBuffer:String = req.ResponseText().Replace("~r", "")
		
				'process buffer
				Local lines:= httpBuffer.Split("~n")
				
				saveSuccess = False
				For Local index:= 0 Until lines.Length()
					If lines[index] = "<SAVED>"
						saveSuccess = True
						Exit
					EndIf
				Next
			
				if saveSuccess
					'success saving
					ResetEditor()
					
					'queue immediate load
					LoadIcons()
				Else
					'error saving
				EndIf
			
			Case MODE_LOAD
				'add data to http buffer
				Local httpBuffer:String = req.ResponseText().Replace("~r", "")
				
				'process buffer
				Local lines:= httpBuffer.Split("~n")
				Local index:Int
		
				For index = 0 Until lines.Length() Step 6
					If lines[index] = "<ICON>" And index + 5 < lines.Length()
						AddIcon(lines[index + 1], lines[index + 2], lines[index + 3], lines[index + 4])
						
						'remember tiemstamp so we dont load any after
						If lines[index + 5] > loadRemoteTimestamp
							loadRemoteTimestamp = lines[index + 5]
						EndIf
					EndIf
				Next
			
				loadTimestamp = ms
		End
		
		'reset http buffer
		httpMode = MODE_IDLE
		busyEndTimestamp = ms
	End
		
	'api
	Method LoadIcons:Void()
		' --- this will load icons from the server ---
		if httpMode = MODE_IDLE
			'we can process a load now
			loadTimestamp = ms
			httpMode = MODE_LOAD
			If loadRemoteTimestamp.Length
				http = New HttpRequest("GET",REMOTE_SERVER + "?mode=load&timestamp=" + loadRemoteTimestamp, Self)
			Else
				http = New HttpRequest("GET", REMOTE_SERVER + "?mode=load", Self)
			EndIf
			http.Send()
			busyText = "Refreshing Icons..."
		Else
			'we can schedule teh load once the save has finished
			if httpMode <> MODE_LOAD loadNext = true
		EndIf
	End
	
	Method SaveIcon:Void()
		' --- this will start a save process ---
		'it can also queue it if there is a load currently
		If httpMode = MODE_IDLE
			'get data fro meditor
			Local author:String = CleanString(editorAuthorInput.Value())
			Local titleRaw:String = editorTitleInput.Value()
			Local title:String = CleanString(titleRaw)
			Local data:String
			
			For Local index:Int = 0 until editorPixels.Length
				data = data + editorPixels[index]
			Next
			
			'start http request
			saveSuccess = False
			httpMode = MODE_SAVE
			http = New HttpRequest("GET", REMOTE_SERVER + "?mode=save&author=" + author + "&title=" + title + "&data=" + data, Self)
			http.Send()
			busyText = "Saving Your Icon '" + titleRaw + "' ..."
		Else
			'save later
			saveNext = True
		EndIf
		
		'disable editor always
		editorDisabled = true
		TextInput.AllInactive()
		ButtonInput.AllInactive()
	End
	
	Method AddIcon:Void(id:String, author:String, title:String, data:String)
		' --- add a new icon so we can process it later ---
		Local icon:Icon
		
		'so we should skip ids that already exist
		For icon = EachIn iconsPending
			if icon.id = id Return
		Next
		
		For icon = EachIn icons
			if icon.id = id Return
		Next
		
		Print "adding icon: " + title + " (by: " + author + ")"
		
		'create new icon
		icon = New Icon
		icon.id = id
		icon.author = author
		icon.title = title
		icon.data = data
		
		Local angle:= Rnd(0, 360)
		Local speed:Float = Rnd(0.1, 0.5)
		icon.x = 320
		icon.y = DeviceHeight() / 2
		icon.vx = (Cos(angle) * speed)
		icon.vy = (Sin(angle) * speed)
		
		iconsPending.AddLast(icon)
	End
	
	Method ResetEditor:Void()
		' --- reset teh editor ---
		'but we keep author field
		editorPixels = New Int[ICON_SIZE * ICON_SIZE]
		editorTitleInput.Clear()
	End
End

'helper classes
Class Icon
	Field id:String
	Field author:String
	Field title:String
	Field data:String
	Field image:Image
	Field x:Float
	Field y:Float
	Field vx:Float
	Field vy:Float
	Field scale:Float = 4
	Field angle:Float = -45
	Field appear:Float = 1.0
End

Class TextInput
	Global all:= new List<TextInput>
	
	Const PADDING:Int = 4
	Const SPACING:Int = 2
	
	Field active:Int
	
	Field title:String
	Field x:Int
	Field y:Int
	Field width:Int
	Field height:Int
	Field maxLength:Int = 0
	
	Field keyboardBuffer:String = ""
	Field keyboardCursorState:= false
	field keyboardCursorTime:Int
	
	'globals
	Function AllInactive:Void()
		' --- make all inputs inactive ---
		For Local input:= eachin all
			input.active = False
		Next
	End
	
	Function AllRender:Void()
		For Local input:= eachin all
			input.Render()
		Next
	End
	
	Function AllUpdate:Void()
		For Local input:= eachin all
			input.Update()
		Next
	End
	
	'methods
	Method New(title:String, x:Int, y:Int, width:Int, height:Int, maxLength:Int = 0)
		' --- setup new ---
		Self.title = title
		Self.x = x
		Self.y = y
		Self.width = width
		Self.height = height
		Self.maxLength = maxLength
		
		all.AddLast(Self)
	End
	
	Method Update:Void()
		' --- update ---
		if active = False
			if MouseHit(MOUSE_LEFT) And MouseX() >= x And MouseX() <= x + width And MouseY() >= y And MouseY() <= y + height
				AllInactive()
				active = true
			EndIf
		Else
			'do input
			Local char := GetChar()
			While char
				Select char
					Case CHAR_TAB
						'switch to next input
						if all.Count() > 1
							'find next node
							Local getNext:Int
							Local nextInput:TextInput
							For Local input:= eachin all
								if getNext = False
									if input = Self getNext = true
								Else
									nextInput = input
									Exit
								EndIf
							Next
							if getNext And nextInput = null nextInput = all.First()
							
							'activate it
							AllInactive()
							nextInput.active = True
							
							'escape this update
							return
						EndIf
						
					Case CHAR_ENTER
						'return
						if keyboardBuffer.Length
							'keyboardBuffer = ""
						EndIf
					Case CHAR_DELETE,CHAR_BACKSPACE
						'delete
						keyboardBuffer = keyboardBuffer[..keyboardBuffer.Length-1]
					
					Default
						keyboardBuffer += String.FromChar(char)
				End
				
				'next get char
				char = GetChar()
			wend
			
			'limit buffer
			if maxLength > 0 and keyboardBuffer.Length > maxLength keyboardBuffer = keyboardBuffer[ .. maxLength]
		EndIf
		
		'do cursor
		if keyboardCursorTime + 200 < IconFun.ms
			keyboardCursorState = Not keyboardCursorState
			keyboardCursorTime = IconFun.ms
		EndIf
	End
	
	Method Render:Void()
		'--- render the input---
		SetFont(IconFun.fontBlack)
		Local fontWidth:= TextWidth("M")
		Local fontHeight:= FontHeight()
		
		'work out input box size
		Local titleWidth:Int
		Local boxWidth:Int
		Local boxX:Int
		if title.Length = 0
			titleWidth = 0
			boxWidth = width
			boxX = x
		Else
			titleWidth = TextWidth(title)
			boxWidth = width - SPACING - titleWidth
			boxX = x + titleWidth + SPACING
		EndIf
		
		SetColor(255, 255, 255)
		DrawRect(boxX, y, boxWidth, height)
		SetColor(0, 0, 0)
		DrawLineRect(boxX, y, boxWidth, height)
		SetColor(255, 255, 255)
		
		'get correct length text
		Local text:String
		
		If TextWidth(keyboardBuffer) >= boxWidth - PADDING - PADDING - SPACING - fontWidth
			text = keyboardBuffer[keyboardBuffer.Length - Floor( (boxWidth - PADDING - PADDING - SPACING - fontWidth) / fontWidth) ..]
		Else
			text = keyboardBuffer
		EndIf
		Local textWidth:= TextWidth(text)
		
		'draw bits now
		'title
		if title.Length
			SetFont(IconFun.fontWhite)
			DrawText(title, int(x), int(y + (height / 2) - (fontHeight / 2)))
		EndIf
		
		'text
		SetFont(IconFun.fontBlack)
		DrawText(text, int(boxX + PADDING), int(y + (height / 2) - (fontHeight / 2)))
		
		'cursor
		if active
			If keyboardCursorState
				SetColor(255,255,255)
			Else
				SetColor(55,55,55)
			EndIf
			DrawRect(boxX + PADDING + SPACING + textWidth, y + (height / 2) - (fontHeight / 2), fontWidth, fontHeight)
		EndIf
	End
	
	Method Value:String()
		Return keyboardBuffer
	End
	
	Method HasValue:Bool()
		' --- returns true if has value ---
		Return keyboardBuffer.Length > 0
	End
	
	Method Clear:Void()
		keyboardBuffer = ""
	End
End

Class ButtonInput
	Global all:= new List<ButtonInput>
	
	Field active:Int = true
	Field hover:Int = false
	Field held:Int = false
	Field pressed:Int = false
	
	Field title:String
	Field x:Int
	Field y:Int
	Field width:Int
	Field height:Int
	
	Field borderColor:Int[3]
	Field backgroundColor:Int[3]
	Field fontColor:Image
	
	'globals
	Function AllInactive:Void()
		' --- make all inputs inactive ---
		For Local input:= eachin all
			input.active = False
			input.held = false
		Next
	End
	
	Function AllRender:Void()
		For Local input:= eachin all
			input.Render()
		Next
	End
	
	Function AllUpdate:Void()
		For Local input:= eachin all
			input.Update()
		Next
	End
	
	'methods
	Method New(title:String, x:Int, y:Int, width:Int, height:Int)
		' --- setup new ---
		Self.title = title
		Self.x = x
		Self.y = y
		Self.width = width
		Self.height = height
		
		all.AddLast(Self)
	End
	
	Method Update:Void()
		' --- update the button ---
		'reset pressed flag
		pressed = False
		
		'update hover
		hover = MouseX() >= x And MouseY() >= y And MouseX() <= x + width And MouseY() <= y + height
		
		if active
			if held = False
				if MouseHit(MOUSE_LEFT) And hover
					held = True
				EndIf
			Else
				if MouseDown(MOUSE_LEFT) = False
					held = False
					if hover pressed = True
				EndIf
			EndIf
		EndIf
	End
	
	Method Render:Void()
		'--- render the button---
		'figure out color based on state
		if active = False
			'disabled
			borderColor[0] = 64
			borderColor[1] = 64
			borderColor[2] = 64
			backgroundColor[0] = 128
			backgroundColor[1] = 128
			backgroundColor[2] = 128
			fontColor = IconFun.fontBlack
		Else
			if held = False
				if hover = False
					'idle
					borderColor[0] = 36
					borderColor[1] = 137
					borderColor[2] = 204
					backgroundColor[0] = 79
					backgroundColor[1] = 182
					backgroundColor[2] = 249
					fontColor = IconFun.fontWhite
				Else
					'over
					borderColor[0] = 36
					borderColor[1] = 137
					borderColor[2] = 204
					backgroundColor[0] = 120
					backgroundColor[1] = 198
					backgroundColor[2] = 250
					fontColor = IconFun.fontWhite
				EndIf
			Else
				if hover = true
					'held
					borderColor[0] = 45
					borderColor[1] = 147
					borderColor[2] = 46
					backgroundColor[0] = 40
					backgroundColor[1] = 210
					backgroundColor[2] = 42
					fontColor = IconFun.fontWhite
				Else
					'idle
					borderColor[0] = 36
					borderColor[1] = 137
					borderColor[2] = 204
					backgroundColor[0] = 79
					backgroundColor[1] = 182
					backgroundColor[2] = 249
					fontColor = IconFun.fontWhite
				EndIf
			EndIf
		EndIf
		
		'render
		'background
		SetColor(backgroundColor[0], backgroundColor[1], backgroundColor[2])
		DrawRect(x, y, width, height)
		
		'text
		SetColor(255, 255, 255)
		SetFont(fontColor)
		DrawText(title, int(x + (width / 2) - (TextWidth(title) / 2)), int(y + (height / 2) - (FontHeight() / 2)))
		
		'border
		SetColor(borderColor[0], borderColor[1], borderColor[2])
		DrawLineRect(x, y, width, height)
	End
End

'functions
Function CleanString:String(value:String)
	' --- makes a string safe to send ---
	Local newValue:String
	Local char:int
	For Local index:= 0 until value.Length
		char = value[index]
		if (char >= 48 And char <= 57) or (char >= 65 And char <= 90) or (char >= 97 And char <= 122) or char = 32 or char = 95 or char = 45 newValue += String.FromChar(char)
	Next
	Return newValue.Replace(" ", "%20")
End

Function DrawLineRect(x:Int, y:Int, width:Int, height:Int)
	DrawLine(x, y, x + width, y)
	DrawLine(x + width, y, x + width, y + height)
	DrawLine(x + width, y + height - 1, x, y + height - 1)
	DrawLine(x, y + height, x, y)
End

Function ARGB:Int(r:Int, g:Int, b:Int, a:Int = 255)
	return(a Shl 24) | (r Shl 16) | (g Shl 8) | b
End