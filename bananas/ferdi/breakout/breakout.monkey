
Import mojo

Class BrickGame Extends App

	Const SCREEN_WIDTH:Int = 640
	Const SCREEN_HEIGHT:Int = 480

	Const BRICK_WIDTH:Int = 32
	Const BRICK_HEIGHT:Int = 16
	
	Const BOARD_WIDTH:Int = 13
	Const BOARD_HEIGHT:Int = 30
	Const BOARD_SIZE:Int = BOARD_WIDTH * BOARD_HEIGHT
	
	Const BORDER_TOP:Int = 16
	Const BORDER_LEFT:Int = 16
	Const BORDER_RIGHT:Int = SCREEN_HEIGHT - 32 - 16
	
	Const BALL_SPEED:Int = 4
	
	Field board:Int[BOARD_SIZE] 

	Field batX:Int = 0
	Field batY:Int = SCREEN_HEIGHT - 24
	Field batWidth:Int = 64
	Field batHeight:Int = 16
	
	Field ballX:Float = 207.3047 'BORDER_RIGHT
	Field ballY:Float = 81.5195
	Field ballVelX:Float = 0.8 '0.2 * BALL_SPEED
	Field ballVelY:Float = -3.9192 '- Sqrt(1.0 - 0.2 * 0.2) * BALL_SPEED
	Field ballRadius:Int = 5
	Field prevBallX:Float = 0
	Field prevBallY:Float = 0
	
	Field AnimTime:Int = 0
	
	Const EMPTY:Int = 0
	Const WHITE:Int = 1
	Const ORANGE:Int = 2
	Const LIGHT_BLUE:Int = 3
	Const GREEN:Int = 4
	Const RED:Int = 5
	Const BLUE:Int = 6
	Const PINK:Int = 7
	Const YELLOW:Int = 8
	Const GOLD:Int = 9
	Const SILVER:Int = 10
	Const GRAY:Int = 11
	Const DARK_GRAY:Int = 12
	Const INVISIBLE:Int = 13
	Const PURPLE:Int = 14
	Const EXPLODE:Int = 15
	Const EXPLOSION0:Int = 100
	Const EXPLOSION1:Int = 101
	Const EXPLOSION2:Int = 102
	Const EXPLOSION3:Int = 103
	Const EXPLOSION4:Int = 104
	Const EXPLOSION5:Int = 105
	Const EXPLOSION6:Int = 106
	Const EXPLOSION7:Int = 107
	Const EXPLOSION8:Int = 108
	Const EXPLOSION9:Int = 109
	Const WAIT_EXPLODE0:Int = 200
	Const WAIT_EXPLODE1:Int = 201
	
	Field explodeWaitTime:Int = 0
	Field waitExplodeFlag:Int = 0
	
	Field level:Int = 1
	
	Field canShoot:Bool = True
	Const MAX_BULLET:Int = 100
	Const BULLET_RADIUS:Int = 3
	Field bulletActive:Bool[MAX_BULLET]
	Field bulletX:Int[MAX_BULLET]
	Field bulletY:Int[MAX_BULLET]
	
	Field ballStickOnBrick:Bool = True
	Const CAN_SHOOT_BRICK_LEFT:Int = 30
	
	Field ballBatBounce:Int = 0
	
	Method OnCreate ()
		SetUpdateRate 60
		SetupBoard()
	End Method

	Method CopyBoard:Void(lvl:Int[])
	
		Local i:Int = 0
		
		For i = 0 Until board.Length
			board[i] = 0
		End For
		
		For i = 0 Until lvl.Length
			board[i] = lvl[i]
		End For
	
	End Method
	
	Method SetupBoard:Void()
	
		Local i:Int = 0
		Local j:Int = 0

		For i = 0 Until MAX_BULLET
			bulletActive[i] = False
		End For

		For i = 0 Until BOARD_SIZE
			board[i] = EMPTY
		End For
		
		Select level
		Case 1
			For i = 0 Until 13
				board[3*BOARD_WIDTH+i] = SILVER
				board[4*BOARD_WIDTH+i] = EXPLODE
				board[5*BOARD_WIDTH+i] = RED
				board[6*BOARD_WIDTH+i] = BLUE
				board[7*BOARD_WIDTH+i] = ORANGE
				board[8*BOARD_WIDTH+i] = EXPLODE
				board[9*BOARD_WIDTH+i] = PURPLE
				board[10*BOARD_WIDTH+i] = GREEN
			End For
		Case 2
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				15, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 1,15, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 1, 2,15, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 1, 2, 3,15, 5, 0, 0, 0, 0, 0, 0, 0, 0,
				 1, 2, 3, 4,15, 6, 0, 0, 0, 0, 0, 0, 0,
				 1, 2, 3, 4, 5,15, 7, 0, 0, 0, 0, 0, 0,
				 1, 2, 3, 4, 5, 6,15, 8, 0, 0, 0, 0, 0,
				 1, 2, 3, 4, 5, 6, 7, 8, 7, 0, 0, 0, 0,
				 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 0, 0, 0,
				 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 0, 0,
				15,15,15,15,15,15,15,15,15,15,15,15,15,
				12,12,12,12,12,12,12,12,12,12,12,12,12]
			CopyBoard(level)
		Case 3
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 5, 7, 4, 3, 6, 0, 5, 7, 4, 3, 6, 0,
				 0,15, 5, 7, 4, 3, 0,15, 5, 7, 4, 3, 0,
				 0, 5,15, 5, 7, 4, 0, 5,15, 5, 7, 4, 0,
				 0, 7, 5,15, 5, 7, 0, 7, 5,15, 5, 7, 0,
				 0, 4, 7, 5,15, 5, 0, 4, 7, 5,15, 5, 0,
				 0, 3, 4, 7, 5,15, 0, 3, 4, 7, 5,15, 0,
				 0, 6, 3, 4, 7, 5, 0, 6, 3, 4, 7, 5, 0,
				 0, 6, 3, 4, 7, 5, 0, 6, 3, 4, 7, 5, 0,
				 0, 3, 4, 7, 5,15, 0, 3, 4, 7, 5,15, 0,
				 0, 4, 7, 5,15, 5, 0, 4, 7, 5,15, 5, 0,
				 0, 7, 5,15, 5, 7, 0, 7, 5,15, 5, 7, 0,
				 0, 5,15, 5, 7, 4, 0, 5,15, 5, 7, 4, 0,
				 0,15, 5, 7, 4, 3, 0,15, 5, 7, 4, 3, 0,
				 0, 5, 7, 4, 3, 6, 0, 5, 7, 4, 3, 6, 0]
			CopyBoard(level)
		Case 4
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 7, 5, 7, 4, 3, 0, 0, 0, 0,
				 0, 0, 0, 7, 5,15,15,15, 6, 3, 0, 0, 0,
				 0, 0, 0, 5,15, 4, 3, 6,15, 4, 0, 0, 0,
				 0, 0, 5, 7,15, 3, 6, 3,15, 7, 5, 0, 0,
				 0, 0, 7,15, 3, 6, 3, 4, 7,15, 7, 0, 0,
				 0, 0, 4,15, 6, 3, 4, 7, 5, 7, 4, 0, 0,
				 0, 0, 3,15, 3, 4, 7, 5, 7, 4, 3, 0, 0,
				 0, 0, 6,15, 4, 7,15, 7, 4, 3, 6, 0, 0,
				 0, 0, 3,15, 7, 5, 7,15, 3, 6, 3, 0, 0,
				 0, 0, 4,15, 5, 7, 4, 3,15, 3, 4, 0, 0,
				 0, 0, 7, 5, 7, 4, 3, 6, 3,15, 7, 0, 0,
				 0, 0, 0, 7, 4, 3, 6, 3, 4, 7, 0, 0, 0,
				 0, 0, 0, 4, 3, 6, 3, 4, 7, 5, 0, 0, 0,
				 0, 0, 0, 0, 6, 3, 4, 7, 5, 0, 0, 0, 0]
			CopyBoard(level)
		Case 5
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 4, 7, 5, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 4, 7,15, 7, 4, 0, 0, 0, 0,
				 0, 0, 0, 4, 7,15, 7,15, 3, 6, 0, 0, 0,
				 0, 0, 0, 0, 5, 7,15, 3, 6, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 4, 3, 6, 0, 6, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 6, 0, 6, 3, 4, 0, 0,
				 0, 0, 0, 5, 0, 0, 0, 6, 3,15, 7, 5, 0,
				 0, 0, 5, 7, 4, 0, 6, 3,15, 7,15, 7, 5,
				 0, 5, 7,15, 3, 6, 0, 4, 7,15, 7, 5, 0,
				 5, 7,15, 3,15, 3, 4, 0, 5, 7, 5, 0, 0,
				 0, 4, 3,15, 3, 4, 0, 0, 0, 5, 0, 0, 0,
				 0, 0, 6, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0]
			CopyBoard(level)
		Case 6
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 0, 0,
				 0, 0,15,15,15,15, 9,15,15,15,15, 0, 0,
				 0, 0, 7, 4, 3, 6, 9, 5, 7, 4, 3, 0, 0,
				 0, 0, 7, 4, 3, 6, 9, 5, 7, 4, 3, 0, 0,
				 0, 0, 7, 4, 3, 6, 9, 5, 7, 4, 3, 0, 0,
				 0, 0, 7, 4, 3, 6, 9, 5, 7, 4, 3, 0, 0,
				 0, 0, 7, 4, 3, 6, 9, 5, 7, 4, 3, 0, 0,
				 0, 0, 7, 4, 3, 6, 9, 5, 7, 4, 3, 0, 0,
				 0, 0, 7, 4, 3, 6, 9, 5, 7, 4, 3, 0, 0,
				 0, 0,15,15,15,15, 9,15,15,15,15, 0, 0,
				 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 0, 0]
			CopyBoard(level)
		Case 7
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 7,15, 3, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 7,15, 3, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 5, 7,15, 3, 6, 0, 0, 0, 0,
				 0, 0, 0, 0, 5, 7,15, 3, 6, 0, 0, 0, 0,
				 0, 0, 0, 7, 5,15,15,15, 6, 3, 0, 0, 0,
				 0, 0, 0, 7, 5,15, 4,15, 6, 3, 0, 0, 0,
				 0, 0, 4, 7,15, 7, 4, 3,15, 3, 4, 0, 0,
				 0, 0, 4, 7,15, 7, 4, 3,15, 3, 4, 0, 0,
				 0, 3, 4,15, 5, 7, 4, 3, 6,15, 4, 7, 0,
				 0, 3, 4,15, 5, 7, 4, 3, 6,15, 4, 7, 0,
				 6, 3,15, 7, 5, 7, 4, 3, 6, 3,15, 7, 5,
				 6, 3,15, 7, 5, 7, 4, 3, 6, 3,15, 7, 5,
				12,12,12,12,12,12,12,12,12,12,12,12,12]
			CopyBoard(level)
		Case 8
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
				 4,15, 4, 4,15, 4, 4, 4,15, 4, 4,15, 4,
				 6, 6,15,15, 6,15, 6,15, 6,15,15, 6, 6,
				 9, 9, 9, 9, 9, 5,15, 5, 9, 9, 9, 9, 9,
				 9, 4, 4, 4, 9,13,13,13, 9, 4, 4, 4, 9,
				 9, 5, 5, 5, 9, 0, 0, 0, 9, 6, 6, 6, 9,
				 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
				 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
				 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
				 9, 0, 0, 0, 9,15,15,15, 9, 0, 0, 0, 9,
				 9,13,13,13, 9, 9, 9, 9, 9,13,13,13, 9]
			CopyBoard(level)
		Case 9
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 5, 5, 5, 0, 5, 5, 5, 0, 5, 5, 5, 0,
				 0, 7,15, 7, 0, 7,15, 7, 0, 7,15, 7, 0,
				 0, 4,15, 4, 0, 4,15, 4, 0, 4,15, 4, 0,
				 0, 3,15, 3, 0, 3,15, 3, 0, 3,15, 3, 0,
				 0, 6,15, 6, 0, 6,15, 6, 0, 6,15, 6, 0,
				 0, 3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3, 0,
				 0, 4, 4, 4, 0, 4, 4, 4, 0, 4, 4, 4, 0,
				 0, 7, 7, 7, 0, 7, 7, 7, 0, 7, 7, 7, 0,
				 0, 5,15, 5, 0, 5,15, 5, 0, 5,15, 5, 0,
				 0, 7,15, 7, 0, 7,15, 7, 0, 7,15, 7, 0,
				 0, 4,15, 4, 0, 4,15, 4, 0, 4,15, 4, 0,
				 0, 3,15, 3, 0, 3,15, 3, 0, 3,15, 3, 0,
				 0, 6, 6, 6, 0, 6, 6, 6, 0, 6, 6, 6, 0,
				 0, 3, 3, 3, 0, 3, 3, 3, 0, 3, 3, 3, 0]
			CopyBoard(level)		
		Case 10
			Local level:Int[] = [
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				15,15,15,15, 4, 9, 0, 9, 4,15,15,15,15,
				 4, 4, 4, 4, 4, 9, 0, 9, 4, 4, 4, 4, 4,
				12,12,12,12,12, 9, 0, 9,12,12,12,12,12,
				 7, 7, 7, 7, 7, 9, 0, 9, 7, 7, 7, 7, 7,
				 5,15, 5,15, 5, 9, 0, 9, 5,15, 5,15, 5,
				 7,15, 7,15, 7, 9, 0, 9, 7,15, 7,15, 7,
				 4, 4,15, 4, 4, 9, 0, 9, 4, 4,15, 4, 4,
				 3,15, 3,15, 3, 9, 0, 9, 3,15, 3,15, 3,
				 6,15, 6,15, 6, 9, 0, 9, 6,15, 6,15, 6,
				 3, 3, 3, 3, 3, 9, 0, 9, 3, 3, 3, 3, 3]
			CopyBoard(level)
		Default
			For j = 0 Until 13
				For i = 0 Until 13
					Local brick:Int = Rnd(15)+1
					If brick = GOLD Then
						brick = EXPLODE
					End If
					board[j*BOARD_WIDTH+i] = brick
				End For
			End For
		End Select
		
		ballStickOnBrick = True
		canShoot = False
		ballBatBounce = 0
	
	End Method
	
	Method BallBrickCollide:Bool(ballX:Float, ballY:Float, brickX1:Float, brickY1:Float, brickX2:Float, brickY2:Float)
	
		If ballX >= brickX1 And ballX <= brickX2 And
			ballY >= brickY1 And ballY <= brickY2 Then
			
			Return True

		End If
		
		Return False

	End Method
	
	Method CheckBrick:Void(i:Int, j:Int)

		If i < 0 Then
			Return
		End If
		
		If j < 0 Then
			Return
		End If
		
		If i >= BOARD_WIDTH Then
			Return
		End If
		
		If j >= BOARD_HEIGHT Then
			Return
		End If

		Select board[j * BOARD_WIDTH + i]
		Case GOLD
			board[j * BOARD_WIDTH + i ] = GOLD			
		Case INVISIBLE
			board[j * BOARD_WIDTH + i ] = PURPLE
		Case DARK_GRAY
			board[j * BOARD_WIDTH + i ] = GRAY
		Case GRAY
			board[j * BOARD_WIDTH + i ] = SILVER
		Case EXPLODE
			Local flag:Int = 1 - waitExplodeFlag
			board[j * BOARD_WIDTH + i ] = WAIT_EXPLODE0 + flag
		Case WAIT_EXPLODE0
			board[j * BOARD_WIDTH + i ] = WAIT_EXPLODE0
		Case WAIT_EXPLODE1
			board[j * BOARD_WIDTH + i ] = WAIT_EXPLODE1
		Default
			board[j * BOARD_WIDTH + i ] = EXPLOSION0
		End Select

	End Method

	Method CheckBrickExplode:Void(i:Int, j:Int)

		If i < 0 Then
			Return
		End If
		
		If j < 0 Then
			Return
		End If
		
		If i >= BOARD_WIDTH Then
			Return
		End If
		
		If j >= BOARD_HEIGHT Then
			Return
		End If

		Select board[j * BOARD_WIDTH + i]
		Case EXPLODE
			Local flag:Int = 1 - waitExplodeFlag
			board[j * BOARD_WIDTH + i ] = WAIT_EXPLODE0 + flag
		Case WAIT_EXPLODE0
			board[j * BOARD_WIDTH + i ] = WAIT_EXPLODE0
		Case WAIT_EXPLODE1
			board[j * BOARD_WIDTH + i ] = WAIT_EXPLODE1
		Default
			board[j * BOARD_WIDTH + i ] = EXPLOSION0
		End Select

	End Method

	Method OnUpdate ()
	
		If KeyHit(KEY_R) Then
			SetupBoard()
		End If
	
		Local countBrick:Int = CountBrick()
		
		If countBrick <= 0 Then
			level += 1
			SetupBoard()
		End If
		
		If countBrick <= CAN_SHOOT_BRICK_LEFT Then
			canShoot = True
		Else
			canShoot = False
		End If

		batX = MouseX() - batWidth / 2
		batY = SCREEN_HEIGHT - 24

		If batX <= BORDER_LEFT Then
			batX = BORDER_LEFT
		End If

		If batX >= BORDER_RIGHT - batWidth Then
			batX = BORDER_RIGHT - batWidth
		End If

		If ballStickOnBrick = True Then

			ballX = batX + batWidth * 3 / 4
			ballY = batY - ballRadius
			
			If MouseHit(MOUSE_LEFT) Then
				ballStickOnBrick = False
			End If
			
			Return
		
		End If

		prevBallX = ballX
		prevBallY = ballY
		
		ballX += ballVelX
		ballY += ballVelY
		
		If ballX - ballRadius <= BORDER_LEFT Then
			If ballVelX < 0 Then
				ballVelX = - ballVelX
			End If
		End If
		If ballX + ballRadius >= BORDER_RIGHT Then
			If ballVelX > 0 Then
				ballVelX = - ballVelX
			End If
		End If
		If ballY - ballRadius <= BORDER_TOP Then
			If ballVelY < 0 Then
				ballVelY = - ballVelY
			End If
		End If
		If ballY >= SCREEN_HEIGHT Then
			SetupBoard()
		End If
		
		If (Millisecs() - explodeWaitTime) > 100 Then
			For Local j:Int = 0 Until BOARD_HEIGHT
				For Local i:Int = 0 Until BOARD_WIDTH
					If board[j * BOARD_WIDTH + i] = WAIT_EXPLODE0 + waitExplodeFlag Then

						board[j * BOARD_WIDTH + i] = EXPLOSION0

						CheckBrickExplode(i - 1, j - 1)
						CheckBrickExplode(i    , j - 1)
						CheckBrickExplode(i + 1, j - 1)

						CheckBrickExplode(i - 1, j)
						CheckBrickExplode(i + 1, j)

						CheckBrickExplode(i - 1, j + 1)
						CheckBrickExplode(i    , j + 1)
						CheckBrickExplode(i + 1, j + 1)
					End If					
				End For
			End For
			
			waitExplodeFlag = 1 - waitExplodeFlag
			explodeWaitTime = Millisecs()
		End If

		If canShoot = True Then
		
			If MouseHit(MOUSE_LEFT) = True Then

				For Local j:Int = 0 Until 2
							
					Local found:Int = -1
	
					For Local i:Int = 0 Until MAX_BULLET
						If bulletActive[i] = False Then
							found = i
							Exit
						End If
					End For
					
					If found >= 0 And found < MAX_BULLET Then
					
						bulletActive[found] = True
						bulletX[found] = batX + j * batWidth + BULLET_RADIUS - j * 2 * (BULLET_RADIUS)
						bulletY[found] = batY - 4
					
					End If
					
				End For
				
			End If
		
		End If
		
		For Local k:Int = 0 Until MAX_BULLET
			If bulletActive[k] = True Then
				bulletY[k] -= 4
				If bulletY[k] < 0 Then
					bulletActive[k] = False
				End If
				For Local j:Int = 0 Until BOARD_HEIGHT
		
					For Local i:Int = 0 Until BOARD_WIDTH
		
						Local brick:Int = board[j * BOARD_WIDTH + i]
		
						If brick <> EMPTY And brick < EXPLOSION0 Then
		
							Local brickLeft:Int = i * BRICK_WIDTH + BORDER_LEFT
							Local brickRight:Int = brickLeft + BRICK_WIDTH - 1
							Local brickTop:Int = j * BRICK_HEIGHT + BORDER_TOP
							Local brickBottom:Int = brickTop + BRICK_HEIGHT - 1
			
							If BallBrickCollide(bulletX[k], bulletY[k], brickLeft, brickTop, brickRight, brickBottom) = True Then
								CheckBrick(i, j)
								bulletActive[k] = False
								Exit
							End If
						End If
					End For
				End For
			End If
		End For
		
		If ballX > batX And ballX < batX + batWidth And
			ballY + ballRadius > batY And ballY + ballRadius < batY + batHeight Then
			
			Local w2:Float = batWidth / 2

			ballVelX = ((ballX - batX - w2) / w2)
			If Abs(ballVelX) < 0.2 Then
				ballVelX = 0.2 * Sgn(ballVelX)
			End If
			If Abs(ballVelX) > 0.9 Then
				ballVelX = 0.9 * Sgn(ballVelX)
			End If
			
			ballVelY = - Sqrt(1.0 - ballVelX * ballVelX)
			
			ballVelX *= BALL_SPEED
			ballVelY *= BALL_SPEED
			
			ballBatBounce += 1
		End If
		
		If (Millisecs() - AnimTime) > 20 Then
		
			For Local j:Int = 0 Until BOARD_HEIGHT

				For Local i:Int = 0 Until BOARD_WIDTH

					Local brick:Int = board[j * BOARD_WIDTH + i]

					If brick <> WAIT_EXPLODE0 And brick <> WAIT_EXPLODE1 Then
						If brick >= EXPLOSION0 Then
							If brick >= EXPLOSION9 Then
								board[j * BOARD_WIDTH + i] = EMPTY
							Else
								board[j * BOARD_WIDTH + i] += 1
							End If
						End If
					End If

				End For

			End For

			AnimTime = Millisecs()
			
		End If
		
		For Local j:Int = 0 Until BOARD_HEIGHT

			For Local i:Int = 0 Until BOARD_WIDTH

				Local brick:Int = board[j * BOARD_WIDTH + i]

				If brick <> EMPTY And brick < EXPLOSION0 Then

					Local brickLeft:Int = i * BRICK_WIDTH + BORDER_LEFT
					Local brickRight:Int = brickLeft + BRICK_WIDTH - 1
					Local brickTop:Int = j * BRICK_HEIGHT + BORDER_TOP
					Local brickBottom:Int = brickTop + BRICK_HEIGHT - 1

					Local hit:Bool = False

					If ballVelX < 0 And BallBrickCollide(ballX - ballRadius, ballY, brickLeft, brickTop, brickRight, brickBottom) = True Then
					
						CheckBrick(i, j)
						ballVelX = - ballVelX
						hit = True

					Else If ballVelX > 0 And BallBrickCollide(ballX + ballRadius, ballY, brickLeft, brickTop, brickRight, brickBottom) = True Then

						CheckBrick(i, j)
						ballVelX = - ballVelX
						hit = True

					End If

					If ballVelY < 0 And BallBrickCollide(ballX, ballY - ballRadius, brickLeft, brickTop, brickRight, brickBottom) = True Then

						CheckBrick(i, j)
						ballVelY = - ballVelY
						hit = True

					Else If ballVelY > 0 And BallBrickCollide(ballX, ballY + ballRadius, brickLeft, brickTop, brickRight, brickBottom) = True Then
					
						CheckBrick(i, j)
						ballVelY = - ballVelY
						hit = True

					End If
					
					If hit = True Then
					
						ballX = prevBallX
						ballY = prevBallY

					End If					

				End If

			End For
			
		End For
		
	
	End Method

	Method DrawBrick:Void(x:Int, y:Int, border:Int = 1)
	
		DrawRect(x + border, y + border, BRICK_WIDTH - border * 2, BRICK_HEIGHT - border * 2)
	
	End Method

	Method DrawGold:Void(x:Int, y:Int)
	
		SetColor(255,192,0)
		DrawRect(x + 1, y + 1, BRICK_WIDTH - 2, 4)
		SetColor(255,255,0)
		DrawRect(x + 1, y + 1 + 4, BRICK_WIDTH - 2, 6)
		SetColor(255,128,0)
		DrawRect(x + 1, y + 1 + 10, BRICK_WIDTH - 2, 4)
	
	End Method
	
	Method DrawRectLine:Void(x:Int, y:Int, w:Int, h:Int)
	
		DrawLine(x,y,x+w,y)
		DrawLine(x,y+h,x+w,y+h)
		DrawLine(x,y,x,y+h)
		DrawLine(x+w,y,x+w,y+h)
	
	End Method
	
	Method CountBrick:Int()

		Local i:Int = 0
		Local j:Int = 0
		Local c:Int = 0
	
		For j = 0 Until BOARD_HEIGHT
			For i = 0 Until BOARD_WIDTH
				Local brick:Int = board[j*BOARD_WIDTH+i]
				If brick <> GOLD And brick <> EMPTY Then
					c += 1
				End If
			End For
		End For
		
		Return c
				
	End Method

	Method OnRender ()
	
		Local i:Int = 0
		Local j:Int = 0
		Local countBrick:Int = CountBrick()
	
		Cls 0, 0, 0
		
		If countBrick <= CAN_SHOOT_BRICK_LEFT Then
			DrawText("Brick Left: " + countBrick, SCREEN_HEIGHT / 2, SCREEN_HEIGHT * 3 / 4 - 20, 0.5, 0)
			DrawText("Press left mouse button to shoot", SCREEN_HEIGHT / 2, SCREEN_HEIGHT * 3 / 4, 0.5, 0)
		End If
		
		'Draw border
		SetColor(128,128,128)
		DrawRect(0,0,16,480)
		DrawRect(0,0,480 -32,16)
		DrawRect(480-16-32,0,16,480)
		
		For j = 0 Until BOARD_HEIGHT
			For i = 0 Until BOARD_WIDTH
			
				Local brick:Int = board[j*BOARD_WIDTH+i]
			
				If brick = GOLD Then
				
					DrawGold(i * BRICK_WIDTH + BORDER_TOP, j * BRICK_HEIGHT + BORDER_LEFT)
				
				Else If brick = EXPLODE Then
				
					SetColor(255,128,0)
					DrawBrick(i * BRICK_WIDTH + BORDER_TOP, j * BRICK_HEIGHT + BORDER_LEFT,1)
					SetColor(255,255,255)
					DrawBrick(i * BRICK_WIDTH + BORDER_TOP, j * BRICK_HEIGHT + BORDER_LEFT,3)
					SetColor(255,128,0)
					DrawBrick(i * BRICK_WIDTH + BORDER_TOP, j * BRICK_HEIGHT + BORDER_LEFT,5)
					SetColor(255,255,255)
					DrawBrick(i * BRICK_WIDTH + BORDER_TOP, j * BRICK_HEIGHT + BORDER_LEFT,7)

				Else If brick <> EMPTY Then

					Select brick
					Case WHITE
						SetColor(255,255,255)
					Case ORANGE
						SetColor(255,128,0)
					Case LIGHT_BLUE
						SetColor(0,128,255)
					Case GREEN
						SetColor(0,255,0)
					Case RED
						SetColor(255,0,0)
					Case BLUE
						SetColor(0,0,255)
					Case PINK
						SetColor(255,128,128)
					Case YELLOW
						SetColor(0,255,255)
					Case GOLD
						SetColor(255,192,0)
					Case SILVER
						SetColor(192,192,192)
					Case GRAY
						SetColor(96,96,96)
					Case DARK_GRAY
						SetColor(32,32,32)
					Case INVISIBLE
						SetColor(0,0,0)
					Case PURPLE
						SetColor(128,0,255)
					Case EXPLODE
						SetColor(255, 128 + Rnd(48), Rnd(100))
					Case EXPLOSION9
						SetColor(0,32,65)				
					Case EXPLOSION8
						SetColor(0,44,89)				
					Case EXPLOSION7
						SetColor(0,56,113)				
					Case EXPLOSION6
						SetColor(0,69,134)			
					Case EXPLOSION5
						SetColor(0,81,158)				
					Case EXPLOSION4
						SetColor(0,93,182)				
					Case EXPLOSION3
						SetColor(0,105,207)				
					Case EXPLOSION2
						SetColor(0,117,231)				
					Case EXPLOSION1
						SetColor(0,130,255)					
					Case EXPLOSION0
						SetColor(28,142,255)
					Case WAIT_EXPLODE0
						SetColor(28,142,255)
					Case WAIT_EXPLODE1
						SetColor(28,142,255)
					End
					
					DrawBrick(i * BRICK_WIDTH + BORDER_TOP, j * BRICK_HEIGHT + BORDER_LEFT)
				End If
			End For
		End For
		
		'Draw the bat
		SetColor(255,128,0)
		DrawRect(batX, batY, batWidth, batHeight)
		
		If canShoot = True Then
			SetColor(255,0,0)
			DrawRect(batX, batY - 1, 6, batHeight + 2)
			DrawRect(batX + batWidth - 6, batY - 1, 6, batHeight + 2)
		End If
		
		For i = 0 Until MAX_BULLET
		
			If bulletActive[i] = True Then
				DrawCircle(bulletX[i], bulletY[i], BULLET_RADIUS)
			End If
		
		End For

		'Draw the ball
		SetColor(255,255,255)
		DrawCircle(ballX,ballY,ballRadius)
		
		If ballStickOnBrick = True Then
			DrawText("Level " + level, SCREEN_HEIGHT / 2, SCREEN_HEIGHT / 2, 0.5, 0.5)
		End If
		
		DrawText("Brick Left: " + countBrick, SCREEN_HEIGHT + 10, 10)
		DrawText("Ball Bat Bounce: " + ballBatBounce, SCREEN_HEIGHT + 10, 30)
		DrawText("R to Restart ", SCREEN_HEIGHT + 10, 50)
		'DrawText("ballX" + ballX, 100, 100)
		'DrawText("ballY" + ballY, 100, 120)
		'DrawText("ballVelX" + ballVelX, 100, 140)
		'DrawText("ballVelY" + ballVelY, 100, 160)
		
	End Method

End Class

Function Main ()
	New BrickGame
End Function


