Strict

Import mojo
Import card
Import progressbar
Import timer
Import helpers
Import fontmachine
Import game

Class GameField Implements CardHideListener, TimerCompleteListener
	
	Field x:Float, y:Float
	
	Field width:Float, height:Float
	
Private
	Global Timers:List<Timer> = New List<Timer>()
	
	'game time in minutes
	Const GAME_TIME:Float = 2
	
	'additional time in seconds
	Const ADDITIONAL_TIME:Int = 5
	
	Const SCORE_CARD:Int = 10
	
	Const SCORE_COMPLETE:Int = 100

	Field cards:Card[]
	
	Field background:Image
	
	Field progressbar:ProgressBar
	
	Field timer:Timer
	
	Field cardsLeft:Int
	
	Field selected:Card[2]
	
	Field score:Int
	
	Field scoreWidth:Float
	
	Field isComplete:Bool
	
Public
	Method New(cols:Int, rows:Int, marginLeft:Float = 20, marginBottom:Float = 20)
		Card.Init("images/cards/", (cols * rows) / 2)
	
		cards = cards.Resize(cols * rows)
		cards[0] = New Card(0, 0, 0)
		
		width = cards[0].width * cols + marginLeft * (cols - 1)
		height = cards[0].height * rows + marginBottom * (rows - 1)
		
		x = (DeviceWidth() -width) * 0.5
		y = (DeviceHeight() -height) * 0.5
		
		Local row:Int = 0
		Local col:Int
		
		For Local i:Int = 1 Until cards.Length()
			col = i Mod cols
		
			If (col = 0) row += 1
			cards[i] = New Card(i, col * cards[0].width + (col * marginLeft), row * cards[0].height + (row * marginBottom))
		Next
		
		background = LoadImage("images/bg.png")
		progressbar = New ProgressBar(x, y + height + 20, width, 5)
		
		timer = New Timer()
		timer.Alarm(GAME_TIME * 60 * 1000, Self)

		scoreWidth = Game.Font.GetTxtWidth(score)
				
		Reset()
		isComplete = False
	End Method
	
	Method Update:Void()
		If (MouseHit() And Timers.IsEmpty()) Then
			If (MouseX() >= x And MouseX() <= x + width And MouseY() >= y And MouseY() <= y + height) Then
				Local mx:Float = MouseX()
				Local my:Float = MouseY()
			
				For Local card:Card = EachIn cards
					If (mx >= x + card.x And mx <= x + card.x + card.width And my >= y + card.y And my <= y + card.y + card.height) Then
						If ( Not card.flipped And card.Flip()) Then
							If (selected[0] = Null) Then
								selected[0] = card
							Else
								selected[1] = card
							End If
						End If
						
						Exit
					End If
				Next
			End If
		End If
		
		timer.Update()
		progressbar.Value = timer.Percent()
	
		For Local card:Card = EachIn cards
			card.Update()
		Next
		
		If (selected[0] <> Null And selected[1] <> Null) Then
			If (selected[1].IsFlipped()) Then
			
				If (selected[0].Type = selected[1].Type) Then
					Local listener:CardHideListener
					cardsLeft -= 2
					
					If (cardsLeft = 0) listener = Self
					
					score += SCORE_CARD
					scoreWidth = Game.Font.GetTxtWidth(score)
					
					(New HideTimer(selected[0], selected[1], listener)).Alarm(250)
				Else
					(New FlipBackTimer(selected[0], selected[1])).Alarm(250)
				End If

				selected[0] = Null
				selected[1] = Null
			End if
		End If
		
		For Local timer:Timer = EachIn Timers
			timer.Update()
		Next
	End Method
	
	Method Draw:Void()
	
		Local by:=0
		While by<DeviceHeight()
			Local bx:=0
			While bx<DeviceWidth()
				DrawImage background,bx,by
				bx+=background.Width()
			Wend
			by+=background.Height()
		wend
		
		progressbar.Draw()
		Game.Font.DrawText(score, (DeviceWidth() -scoreWidth) * 0.5, y - 60)
	
		PushMatrix()
			Translate(x + cards[0].width * 0.5, y + cards[0].height * 0.5)
		
			For Local card:Card = Eachin cards
				card.Draw()
			Next
		PopMatrix()
	End Method
	
	Method Reset:Void()
		If (cardsLeft > 0) Return
	
		cardsLeft = cards.Length()
		
		Local date:Int[] = GetDate()
		Seed = (date[3] * 3600 + date[4] * 60 + date[5]) * 1000 + date[6]
	
		Local numTypes:Int = cards.Length() / 2
		Local typesLeft:Int[numTypes]
		Local typesRight:Int[numTypes]
		
		For Local i:Int = 0 Until numTypes
			typesLeft[i] = i
			typesRight[i] = i
		Next
		
		ArrayHelper<Int>.Shuffle(typesLeft, numTypes * 2)
		ArrayHelper<Int>.Shuffle(typesRight, numTypes * 2)
		
		For Local i:Int = 0 Until numTypes
			cards[i].Type = typesLeft[i]
			cards[i + numTypes].Type = typesRight[i]
		Next

		For Local card:Card = EachIn cards
			card.Show()
		Next
	End Method
	
	Method OnCardHidden:Void()
		'last card hidden
		Reset()
		timer.AddTime(ADDITIONAL_TIME * 1000)
		score += SCORE_COMPLETE
		scoreWidth = Game.Font.GetTxtWidth(score)
	End Method
	
	Method OnTimerComplete:Void()
		'game over
		progressbar.Value = 0
		isComplete = True
	End Method
	
	Method IsComplete:Bool() Property
		Return isComplete
	End Method
	
	Method Score:Int() Property
		Return score
	End Method

End Class

Private

Class ActionTimer Extends Timer Implements TimerCompleteListener Abstract

	Field cards:Card[2]

	Method New(card1:Card, card2:Card)
		cards[0] = card1
		cards[1] = card2
	End Method
	
	Method Alarm:Void(time:Int, onComplete:TimerCompleteListener = Null)
		Super.Alarm(time, Self)
		GameField.Timers.AddLast(Self)
	End Method
	
	Method OnTimerComplete:Void()
		GameField.Timers.RemoveEach(Self)
	End Method

End Class

Class FlipBackTimer Extends ActionTimer

	Method New(card1:Card, card2:Card)
		Super.New(card1, card2)
	End Method
	
	Method OnTimerComplete:Void()
		Super.OnTimerComplete()
		cards[0].Flip()
		cards[1].Flip()
		cards[0] = Null
		cards[1] = Null
	End Method

End Class

Class HideTimer Extends ActionTimer

	Field hideListener:CardHideListener

	Method New(card1:Card, card2:Card, listener:CardHideListener = Null)
		Super.New(card1, card2)
		hideListener = listener
	End Method
	
	Method OnTimerComplete:Void()
		Super.OnTimerComplete()
		If (cards[0].id > cards[1].id) Then
			cards[0].Hide(hideListener)
			cards[1].Hide()
		Else
			cards[0].Hide()
			cards[1].Hide(hideListener)
		End If
		
		cards[0] = Null
		cards[1] = Null
	End Method

End Class