
#rem
An implementation of Conway's Game of Life

http://en.wikipedia.org/wiki/Conway's_Game_of_Life

Not the simplest version, but that's the price 
of glitz and interactivity. ;)

To try different patterns, pause the 'game', click/touch cells,
then unpause.

#end


Strict

Import mojo
Import monkey.random

Function Main:Int()

	New MyApp

	Return 0
	
End


'-------------------------------
Global ACTION_NONE:Int = 0
Global ACTION_PAUSETOGGLE:Int = 1
Global ACTION_SPEEDUP:Int = 2
Global ACTION_SPEEDDOWN:Int = 3
Global ACTION_RESET:Int = 4
Global ACTION_CLEAR:Int = 5
'-------------------------------

Class MyApp Extends App


	Field procCount:Int			'Process count (iterations)
	Field procRate:Float		'Process rate (hz)
	Field procVal:Float			
	Field paused:Bool

	Field map:World				'Map grid
	Field renderSys:RenSys

	Field action:Int


	Method OnCreate:Int()

		procCount = 0
		procVal = 0.0
		procRate = 8.0
		paused = False
		
		'Map creation and initialisation
		map = New World(30, 20)		
		map.Init()

		'Render system creation and initialisation
		renderSys = New RenSys(map)
		renderSys.SetGrid(5, 25, 20, 20, 1)
		Local lbls:String[] = "Pause, Speed+, Speed-, Reset, Clear".Split(",")
		Local tlx:Int = 5
		For Local i:Int = 0 Until lbls.Length
			Local butt:Button = renderSys.AddButton(lbls[i], tlx + (i * 20), DeviceHeight() - 25)
			tlx += butt.width
		Next
		
		renderSys.MakeStatusStr(procCount, procRate, paused)

		SetUpdateRate 30
	
		Return 0
		
	End

	
	Method OnUpdate:Int()
						
		action = ACTION_NONE
		
		If TouchHit(0)
	
			Local tchx:Int = TouchX(0)
			Local tchy:Int = TouchY(0)
			Local butt:Button = renderSys.GetTouchedButton(tchx, tchy)
			
			If butt 
				
				Local str:String = butt.label.ToLower()
				If str.Contains("pause") Then action = ACTION_PAUSETOGGLE
				If str.Contains("clear") Then action = ACTION_CLEAR
				If str.Contains("speed+") Then action = ACTION_SPEEDUP
				If str.Contains("speed-") Then action = ACTION_SPEEDDOWN
				If str.Contains("reset") Then action = ACTION_RESET
					
			Else
				
				Local tcell:Cell = renderSys.GetTouchedCell(tchx, tchy)
			
				If tcell
			
					If tcell.status = CELL_STATUS_ALIVE 
						tcell.status = CELL_STATUS_DEAD
					Else
						tcell.status = CELL_STATUS_ALIVE
					Endif
			
				Endif	
				
			Endif
					
		Endif
					
		If KeyHit(KEY_SPACE) Then action = ACTION_PAUSETOGGLE			
		If KeyHit(KEY_C) Then action = ACTION_CLEAR
		If KeyDown(KEY_EQUALS) Then action = ACTION_SPEEDUP
		If KeyDown(KEY_MINUS) Then action = ACTION_SPEEDDOWN 
		If KeyHit(KEY_ENTER) Then action = ACTION_RESET
	
		Select action
		
			Case ACTION_PAUSETOGGLE
				paused = Not paused
			
			Case ACTION_SPEEDUP
				procRate += 0.5
				If procRate > UpdateRate() Then procRate = UpdateRate()
				
			Case ACTION_SPEEDDOWN
				procRate -= 0.5
				If procRate < 0.5 Then procRate = 0.5
				
			Case ACTION_RESET
				Seed = Millisecs()
				map.Init
				procCount = 0
				procVal = 0.0
			
			Case ACTION_CLEAR
				map.ClearGrid()
		
		End
	
		If (Not paused)
			procVal += procRate
			If procVal >= UpdateRate()
				procVal = 0.0
				procCount += 1
				map.Process()
			Endif
		Endif
	
		renderSys.MakeStatusStr(procCount, procRate, paused)
	
		Return 0
		
	End

	
	Method OnRender:Int()

		renderSys.Render()		
	
		Return 0
		
	End

End


'------------------


Class RenSys
	
	
	'Reference to a previously created and initialised map grid (World class):
	Field map:World			

	'Render start coordinates (top left x, top left y):
	Field gridTLX:Int 		
	Field gridTLY:Int
	
	'Cell rendering size and space in between:
	Field gridRectWidth:Int
	Field gridRectHeight:Int
	Field gridRectBorder:Int

	'Status string
	Field statusStr:String

	'Array of interactive text buttons:
	Field buttons:Button[]


	Method New(map:World)
	
		Self.map = map
		
		SetGrid(5, 5, 2, 2, 0)

	End	
	
	
	Method SetGrid:Void(tlx:Int, tly:Int, rw:Int, rh:Int, rb:Int)
		
		gridTLX = tlx
		gridTLY = tly
		gridRectWidth = rw
		gridRectHeight = rh
		gridRectBorder = rb
	
	End
	
	
	Method MakeStatusStr:Void(procCount:Int, procRate:Float, paused:Bool)
	
		statusStr = "Iterations: " + procCount + "    Speed (hz): "
		
		Local procStr:String = String(procRate)[0..5]
		If Not procStr.Contains(".") Then procStr += ".0"
		
		statusStr += procStr
		
		If paused Then statusStr += " (Paused)"
		
	End
		
	
	Method AddButton:Button(label:String, tlx:Int, tly:Int)
	
		Local font:Image = GetFont()
		Local width:Int = font.Width() 
		Local height:Int = font.Height()
	
		Local butt:Button = New Button(label, tlx, tly, width * label.Length, height)
	
		If butt
			buttons = buttons.Resize(buttons.Length + 1)
			buttons[buttons.Length - 1] = butt
			Return butt
		Endif
	
		Return Null
		
	End
		

	Method GetTouchedButton:Button(x:Int, y:Int)
		
		For Local butt:Button = Eachin buttons	
			If butt.XYInside(x, y)
				Return butt
			Endif
		End
	
		Return Null
	
	End
	

	Method GetTouchedCell:Cell(x:Int, y:Int)
	
		If (x >= gridTLX) And (x < (gridTLX + (map.cols * (gridRectWidth + gridRectBorder)))) 
			If (y >= gridTLY) And (y < (gridTLY + (map.rows * (gridRectHeight + gridRectBorder))))
			
				Local c:Int = Floor((x - gridTLX) / (gridRectWidth + gridRectBorder))
				Local r:Int = Floor((y - gridTLY) / (gridRectHeight + gridRectBorder))
				
				Return map.GetCell(c, r)
			
			Endif
		Endif
		
		Return Null
		
	End
	

	Method Render:Void()
	
		Cls 0, 0, 0
	
		For Local c:Int = 0 Until map.cols
			For Local r:Int = 0 Until map.rows
				Local curCell:Cell = map.GetCell(c, r)
				SetColor 128, 128, 128
				If curCell.status = CELL_STATUS_ALIVE
					SetColor 128, 255, 128
				Endif
				DrawRect gridTLX + (c * (gridRectWidth + gridRectBorder)) , gridTLY + (r * (gridRectHeight + gridRectBorder)) , gridRectWidth, gridRectHeight
			Next
		Next
		
		SetColor 255, 255, 255
		
		DrawText statusStr, 5, 5
		
		For Local butt:Button = Eachin buttons
			DrawText butt.label, butt.tlx, butt.tly
		Next
		
	End


End


Class Button


	Field label:String
	Field tlx:Int, tly:Int
	Field width:Int, height:Int


	Method New(label:String, tlx:Int, tly:Int, width:Int, height:Int)
	
		Self.label = label
		Self.tlx = tlx
		Self.tly = tly
		Self.width = width
		Self.height = height
	
	End
	
	
	Method XYInside:Button(x:Int, y:Int)
	
		If (x >= tlx) And (x < (tlx + width)) And (y >= tly) And (y < (tly + height))
			Return Self
		Endif
		
		Return Null
		
	End
	

End



'----------------------------------
Global CELL_STATUS_DEAD:Int = 0
Global CELL_STATUS_ALIVE:Int = 1
'----------------------------------


Class World

	'Size of map in columns and rows:
	Field cols:Int
	Field rows:Int

	'Grid is a 1D array cols*rows size.
	Field grid:Cell[]


	Method New(cols:Int, rows:Int)

		Self.cols = cols
		Self.rows = rows

		grid = New Cell[cols * rows]
		For Local c:Int = 0 Until cols
			For Local r:Int = 0 Until rows
				Local i:Int = (r * cols) + c
				grid[i] = New Cell
				grid[i].col = c
				grid[i].row = r
			Next
		Next

	End


	Method PlacePattern:Void(pat:Int[], width:Int, height:Int, atTLCol:Int, atTLRow:Int)
	
		For Local c:Int = 0 Until width
			For Local r:Int = 0 Until height
				If ((atTLCol + c) < cols) And ((atTLRow + r) < rows)
					Local i:Int = ((atTLRow + r) * cols) + (atTLCol + c)
					grid[i].status = pat[(r * width) + c]
				End
			Next
		Next
	
	End


	Method GetCell:Cell(col:Int, row:Int)
	
		If (col < cols) And (col >= 0) And (row < rows) And (row >= 0)
			Return grid[(row * cols) + col]
		Endif
		
		Return Null
		
	End


	Method ClearGrid:Void()	
				
		For Local i:Int = 0 Until grid.Length
			grid[i].status = CELL_STATUS_DEAD
		Next
	
	End


	Method Init:Void()
	
		ClearGrid

		For Local i:Int = 0 Until grid.Length
			'25% of the cells will start off alive
			If Floor(Rnd(0, 3)) = 1 
				grid[i].status = CELL_STATUS_ALIVE
			Endif
		Next
	
	End
	

	Method Process:Void()
		
		For Local i:Int = 0 Until grid.Length
		
			Local curCell:Cell = grid[i]
			curCell.pending = CELL_STATUS_DEAD
			
			Local liveCount:Int = CountLiveNeighbours(curCell.col, curCell.row)
					
			Select curCell.status
				Case CELL_STATUS_ALIVE
					curCell.pending = CELL_STATUS_ALIVE
					If (liveCount < 2) Or (liveCount > 3)
						curCell.pending = CELL_STATUS_DEAD
					Endif
				Case CELL_STATUS_DEAD
					If (liveCount = 3) Then curCell.pending = CELL_STATUS_ALIVE
			End
					
		Next
	
		For Local i:Int = 0 Until grid.Length
			grid[i].status = grid[i].pending
		Next
	
	End


	Method CountLiveNeighbours:Int(cc:Int, cr:Int)
	
		Local count:Int = 0
		
		Local startc:Int = cc - 1
		If startc < 0 Then startc = 0
		
		Local endc:Int = cc + 1
		If endc >= cols Then endc = cols - 1
		
		Local startr:Int = cr - 1
		If startr < 0 Then startr = 0
		
		Local endr:Int = cr + 1
		If endr >= rows Then endr = rows - 1

		For Local c:Int = startc To endc
			For Local r:Int = startr To endr
				If grid[(r * cols) + c].status = CELL_STATUS_ALIVE Then count += 1
			End
		End
		
		If grid[(cr * cols) + cc].status = CELL_STATUS_ALIVE Then count -= 1
		
		Return count
		
	End


End


'----------------------------------


Class Cell

	'Current status:
	Field status:Int
	
	'Pending status to be applied after ALL cells have been 
	'evaluated in the current process 'pass':
	Field pending:Int
	
	'The cells location in the grid.
	Field col:Int
	Field row:Int

	Method New()
		
		status = CELL_STATUS_DEAD
	
	End

End
