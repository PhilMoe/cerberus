Strict

Import mojo
Import tweening

Class Card

	Field x:Float, y:Float
	
	Field width:Float, height:Float
	
	Field flipped:Bool = False
	
	Field hidden:Bool = False
	
	Field id:Int

Private
	Const ANIMATION_NONE:Int = 0
	
	Const ANIMATION_SHOW:Int = 1
	
	Const ANIMATION_HIDE:Int = 2
	
	Const ANIMATION_FLIP:Int = 3
	
	Global Types:Image[]
	
	Global BgImage:Image
	
	Field type:Int
	
	Field image:Image
	
	Field scale:Float = 1, angle:Float = 0, alpha:Float = 0
	
	Field animation:Int = ANIMATION_NONE
	
	Field backward:Bool = False
	
	Field showTween:Tween
	
	Field flipTween:Tween
	
	Field hideTween:Tween
	
	Field hideListener:CardHideListener
	
Public
	Method New(id:Int, x:Float, y:Float)
		Make(x, y)
		width = BgImage.Width()
		height = BgImage.Height()
		
		showTween = New Tween(500, Ease.BounceOut)
		flipTween = New Tween(100, Ease.SineInOut)
		hideTween = New Tween(700, Ease.CubeOut)
		
		Self.id = id
	End Method

	Method Make:Card(x:Float, y:Float)
		Self.x = x
		Self.y = y
		Return Self
	End Method
	
	Method Update:Void()
		If ( Not animation) Return
	
		Select animation
			Case ANIMATION_SHOW
				showTween.Update()
				scale = showTween.Scale
				
				If (scale >= 1) animation = ANIMATION_NONE
				
			Case ANIMATION_HIDE
				hideTween.Update()
				angle = -540 * 1 * hideTween.Scale
				scale = 1 - hideTween.Scale
				alpha = 1 - hideTween.Scale
				
				If (hideTween.Scale >= 1) Then
					animation = ANIMATION_NONE
					hidden = True

					If (hideListener <> Null) Then
						hideListener.OnCardHidden()
						hideListener = Null
					End If
				End If
				
			Case ANIMATION_FLIP
				flipTween.Update()
				
				If ( Not backward) Then
					scale = 1 - 0.9 * flipTween.Scale
				Else
					scale = 0.1 + 0.9 * flipTween.Scale
				End If
				
				If (flipTween.Scale >= 1) Then
					If (backward) Then
						animation = ANIMATION_NONE
					Else
						backward = True
						flipTween.Start()
						flipped = Not flipped
						
						If (flipped) Then
							image = Types[type]
						Else
							image = BgImage
						End If
					End If
				End If
		End Select
	End Method
	
	Method Draw:Void()
		If (hidden) Return
		
		If (animation) Then		
			PushMatrix()
				Translate(x, y)
				
				PushMatrix()
					Select animation
						Case ANIMATION_SHOW
							Scale(scale, scale)
							DrawImage(image, 0, 0)
							
						Case ANIMATION_HIDE
							Scale(scale, scale)
							Rotate(angle)
							SetAlpha(alpha)
							DrawImage(image, 0, 0)
							
							SetAlpha(1)
						Case ANIMATION_FLIP
							Transform(scale, -Abs(1 - scale) * 0.01, 0, 1, 0, 0)
							DrawImage(image, 0, 0)
					End Select
										
				PopMatrix()
			PopMatrix()
			
		Else
			PushMatrix()
				Translate(x, y)
				DrawImage(image, 0, 0)
								
			PopMatrix()
		End If
	End Method
	
	Method Show:Void()
		If (animation) Return
	
		Reset()
		scale = 0
		showTween.Start()
		animation = ANIMATION_SHOW
		hidden = False
		image = BgImage
	End Method
	
	Method Hide:Void(listener:CardHideListener = Null)
		If (animation) Return
	
		Reset()
		hideTween.Start()
		animation = ANIMATION_HIDE
		hideListener = listener
	End Method
	
	Method Flip:Bool()
		If (animation) Return False
	
		Reset()
		flipTween.Start()
		animation = ANIMATION_FLIP
		backward = False
		
		Return( Not flipped)
	End Method
	
	Method IsFlipped:Bool()
		Return(flipped And animation = ANIMATION_NONE)
	End Method
	
	Method Type:Void(type:Int) Property
		Self.type = type
		flipped = False
		hidden = False
		Make(x, y)
	End Method
	
	Method Type:Int() Property
		Return type
	End Method
	
	Function Init:Void(path:String, numTypes:Int)
		BgImage = LoadImage(path + "bg.png",, Image.DefaultFlags | Image.MidHandle)		
		Types = Types.Resize(numTypes)
		
		For Local i:Int = 1 To numTypes
			Types[i - 1] = LoadImage(path + i + ".png",, Image.DefaultFlags | Image.MidHandle)
		Next
	End Function
	
Private
	Method Reset:Void()
		scale = 1
		angle = 0
		alpha = 1
	End Method

End Class

Interface CardHideListener
	
	Method OnCardHidden:Void()

End Interface
