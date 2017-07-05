
Import mojo

Class SnakeGame Extends App

	Const BOARD_WIDTH:Int = 80
	Const BOARD_HEIGHT:Int = 50
	Const BOARD_SIZE:Int = BOARD_WIDTH * BOARD_HEIGHT

	Const NORTH:Int = 1
	Const SOUTH:Int = 2
	Const WEST:Int = 3
	Const EAST:Int = 4
	
	Const EMPTY:Int = 0
	Const WALL:Int = 5000
	Const NUMBER:Int = 10000

	Field board:Int[BOARD_SIZE]
	Field snakeX:Int = 0
	Field snakeY:Int = 0
	Field snakeDir:Int = 0
	Field snakeLen:Int = 2
	Field level:Int = 1
	Field moveSpeed:Int = 0
	
	Field noNum:Bool = True
	Field currNum:Int = 1
	Field grid:Int = 0
	Field warp:Int = 0
	
	Field waitForSpace:Bool = True
	Field waitForSpaceString:String = "Level "

	Method OnCreate ()

		SetUpdateRate 60

		SetupLevel(level)

	End Method
	
	Method SetBoard:Void(x:Int, y:Int, tile:Int)
	
		board[y * BOARD_WIDTH + x] = tile
	
	End Method
	
	Method SetupLevel:Void(lvl:Int, str:String = "")
	
		Local i:Int = 0
		
		For i = 0 Until BOARD_SIZE
		
			board[i] = EMPTY

		End For
		
		If warp = 0 Then 
			For i = 0 Until BOARD_WIDTH
				SetBoard(i, 0, WALL)
				SetBoard(i, BOARD_HEIGHT - 1, WALL)
			End For
			For i = 0 Until BOARD_HEIGHT
				SetBoard(0, i, WALL)
				SetBoard(BOARD_WIDTH - 1, i, WALL)
			End For
		End If
	
		Select lvl
		Case 1
			snakeX = 50
			snakeY = 25
			snakeDir = EAST
		Case 2
			For i = 20 Until 60
				SetBoard(i, 25, WALL)
			End For
			snakeX = 60
			snakeY = 7
			snakeDir = WEST
		Case 3
			For i = 10 Until 40
				SetBoard(20, i, WALL)
				SetBoard(60, i, WALL)
			End For
			snakeX = 50
			snakeY = 25
			snakeDir = NORTH
		Case 4
			For i = 4 to 30
				SetBoard(20, i, WALL)
				SetBoard(60, 53 - i, WALL)
			End For
			For i = 2 To 40
				SetBoard(i, 38, WALL)
				SetBoard(81 - i, 15, WALL)
			End For
			snakeX = 60
			snakeY = 7
			snakeDir = WEST
		Case 5
			For i = 13 To 39
				SetBoard(21, i, WALL)
				SetBoard(59, i, WALL)
			End For
			For i = 23 to 57
				SetBoard(i, 11, WALL)
				SetBoard(i, 41, WALL)
			End For
			snakeX = 50
			snakeY = 25
			snakeDir = NORTH
		Case 6
			For i = 4 To 49
				If i > 30 Or i < 23 Then
					SetBoard(10, i, WALL)
					SetBoard(20, i, WALL)
					SetBoard(30, i, WALL)
					SetBoard(40, i, WALL)
					SetBoard(50, i, WALL)
					SetBoard(60, i, WALL)
					SetBoard(70, i, WALL)				
				End If
			End For
			snakeX = 65
			snakeY = 7
			snakeDir = SOUTH
		Case 7
			For i = 4 To 49 Step 2
				SetBoard(40, i, WALL)
			End For
			snakeX = 65
			snakeY = 7
			snakeDir = SOUTH
		Case 8
			For i = 4 To 40
				SetBoard(10, i, WALL)
				SetBoard(20, 53 - i, WALL)
				SetBoard(30, i, WALL)
				SetBoard(40, 53 - i, WALL)
				SetBoard(50, i, WALL)
				SetBoard(60, 53 - i, WALL)
				SetBoard(70, i, WALL)
			End For
			snakeX = 65
			snakeY = 7
			snakeDir = SOUTH
		Case 9
			For i = 6 To 47
				SetBoard(i, i, WALL)
				SetBoard(i + 28, i, WALL)
			End For
			snakeX = 75
			snakeY = 40
			snakeDir = NORTH
		Default
			For i = 4 To 49 Step 2
				SetBoard(10, i, WALL)
				SetBoard(20, i + 1, WALL)
				SetBoard(30, i, WALL)
				SetBoard(40, i + 1, WALL)
				SetBoard(50, i, WALL)
				SetBoard(60, i + 1, WALL)
				SetBoard(70, i, WALL)
			End For
			snakeX = 65
			snakeY = 7
			snakeDir = SOUTH
		End Select
		
		snakeLen = 2
		noNum = True
		currNum = 1
		
		If str <> "" Then
			waitForSpaceString = str
		Else
			waitForSpaceString = "Level " + level
		End If
		waitForSpace = True
		
	End Method

	Method OnUpdate ()
	
		Local i:Int = 0
		
		If KeyHit(KEY_F1) Then
			grid = 1 - grid
		End If
		
		If KeyHit(KEY_F2) Then
			warp = 1 - warp
			If warp = 1 Then
				For i = 0 Until BOARD_WIDTH
					SetBoard(i, 0, 0)
					SetBoard(i, BOARD_HEIGHT - 1, 0)
				End For
				For i = 0 Until BOARD_HEIGHT
					SetBoard(0, i, 0)
					SetBoard(BOARD_WIDTH - 1, i, 0)
				End For
			Else
				For i = 0 Until BOARD_WIDTH
					SetBoard(i, 0, WALL)
					SetBoard(i, BOARD_HEIGHT - 1, WALL)
				End For
				For i = 0 Until BOARD_HEIGHT
					SetBoard(0, i, WALL)
					SetBoard(BOARD_WIDTH - 1, i, WALL)
				End For						
			End If
		End If

		If waitForSpace = True Then
			If KeyHit(KEY_SPACE) Then
				waitForSpace = False
			Else
				Return
			End If
		End If
		
		noNum = True
		For i = 0 Until BOARD_SIZE
			If board[i] >= NUMBER Then
				noNum = False
			End If
		End For
	
		If noNum = True Then
			'DebugStop()
			Local i:Int = 0
			Repeat
				i = Rnd(BOARD_SIZE)
			Until board[i] = EMPTY
			board[i] = NUMBER + currNum
			noNum = False
		End If
	
		If snakeDir <> SOUTH Then
			If KeyHit(KEY_W) Or KeyHit(KEY_UP) Then
				snakeDir = NORTH
				moveSpeed = Millisecs() - 1000
			End If
		End If
				
		If snakeDir <> NORTH Then
			If KeyHit(KEY_S) Or KeyHit(KEY_DOWN) Then
				snakeDir = SOUTH
				moveSpeed = Millisecs() - 1000
			End If
		End If

		If snakeDir <> EAST Then
			If KeyHit(KEY_A) Or KeyHit(KEY_LEFT) Then
				snakeDir = WEST
				moveSpeed = Millisecs() - 1000
			End If
		End If

		If snakeDir <> WEST Then
			If KeyHit(KEY_D) Or KeyHit(KEY_RIGHT) Then
				snakeDir = EAST
				moveSpeed = Millisecs() - 1000
			End If
		End If

		If (Millisecs() - moveSpeed) > 100 Then

			For Local i:Int = 0 Until BOARD_SIZE
			
				If board[i] > 0 And board[i] < WALL Then
					board[i] -= 1
				End If
				
			End For

			SetBoard(snakeX, snakeY, snakeLen)

			Select snakeDir
			Case NORTH
				snakeY -= 1
			Case SOUTH
				snakeY += 1
			Case WEST
				snakeX -= 1
			Case EAST
				snakeX += 1
			End Select
			
			If snakeX < 0 Then
				snakeX = BOARD_WIDTH - 1
			End If
			If snakeX >= BOARD_WIDTH
				snakeX = 0
			End If
			If snakeY < 0 Then
				snakeY = BOARD_HEIGHT - 1
			End If
			If snakeY >= BOARD_HEIGHT
				snakeY = 0
			End If
			
			If board[snakeY * BOARD_WIDTH + snakeX] >= NUMBER Then

				For Local i:Int = 0 Until BOARD_SIZE
					If board[i] > 0 And board[i] < WALL Then
						board[i] += snakeLen
					End If			
				End For
				snakeLen *= 2
				Print "Snake Length: " + snakeLen

				currNum += 1
				noNum = True
				
				If currNum >= 10 Then
					level += 1
					SetupLevel(level)
				End If
			
			Else If board[snakeY * BOARD_WIDTH + snakeX] <> EMPTY Then
			
				SetupLevel(level, "You Died")
			
			End If
		
			moveSpeed = Millisecs()
		
		End If		
	
	End Method

	Method DrawBox(x:Int, y:Int)
	
		DrawRect(x * 8, y * 8, 8, 8)
	
	End Method

	Method OnRender ()
	
		Local i:Int = 0
		Local j:Int = 0
	
		Cls(0, 0, 0)
		
		' Draw number 1 - 9 first because there is a black rectangle
		' around the font.
		For j = 0 Until BOARD_HEIGHT		
			For i = 0 Until BOARD_WIDTH
				Local tile:Int = board[j * BOARD_WIDTH + i]
				If tile >= NUMBER Then
					Local number = tile - NUMBER
					SetColor(255,255,255)
					DrawText("" + number, i * 8, j * 8 - 2, 0, 0)
				End If
			End For
		End For

		For j = 0 Until BOARD_HEIGHT
		
			For i = 0 Until BOARD_WIDTH
			
				Local tile:Int = board[j * BOARD_WIDTH + i]
				
				If tile = EMPTY Then
				
				Else If tile = WALL Then

					SetColor(255,0,0)
					DrawBox(i, j)
					
				Else If (tile > EMPTY And tile < WALL) Then

					SetColor(255,255,0)
					DrawBox(i, j)
					
				End If
			
			End For
			
		End For
		
		SetColor(255,255,0)
		DrawBox(snakeX, snakeY)
		
		If grid = 1 Then 
			SetColor(64,64,64)
			For i = 1 Until BOARD_WIDTH
				DrawLine(i * 8, 0, i * 8, BOARD_HEIGHT * 8)
			End For
			For j = 1 Until BOARD_HEIGHT
				DrawLine(0, j * 8, BOARD_WIDTH * 8, j * 8)
			End For
			SetColor(255,255,255)
			DrawText("F1 - Grid: On", 0, 50 * 8, 0, 0)
		Else
			SetColor(255,255,255)
			DrawText("F1 - Grid: Off", 0, 50 * 8, 0, 0) 
		End If
		
		SetColor(64,64,64)
		DrawLine(0 * 8, 0, 0 * 8, BOARD_HEIGHT * 8)
		DrawLine(BOARD_WIDTH * 8 - 1, 0, BOARD_WIDTH * 8 - 1, BOARD_HEIGHT * 8)
		DrawLine(0, 0 * 8, BOARD_WIDTH * 8, 0 * 8)
		DrawLine(0, BOARD_HEIGHT * 8 - 1, BOARD_WIDTH * 8, BOARD_HEIGHT * 8 - 1)
		
		If warp = 1 Then
			SetColor(255,255,255)
			DrawText("F2 - Warp: On", 0, 50 * 8 + 13, 0, 0)
		Else
			SetColor(255,255,255)
			DrawText("F2 - Warp: Off", 0, 50 * 8 + 13, 0, 0) 
		End If
		
		If waitForSpace = True Then
			SetColor(255,255,255)
			DrawText(waitForSpaceString, 320, 240 - 26, 0.5, 0) 
			DrawText("Press Space", 320, 240 - 13, 0.5, 0) 
		End If

	End Method

End

Function Main ()
	New SnakeGame
End

