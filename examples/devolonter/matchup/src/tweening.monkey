Strict

Import mojo

Class Tween
	
Private
	Field active:Bool

	Field target:Float
	
	Field ease:EaseFunction
	
	Field t:Float
	
	Field lastTime:Int
	
	Field time:Int

Public
	Method New(duration:Float, ease:EaseFunction = Null)
		target = duration
		Self.ease = ease
		t = 0
	End Method
	
	Method Update:Void()
		If ( Not active) Return
	
		time += (Millisecs() -lastTime)
		lastTime = Millisecs()
		t = time / target
		
		If (ease <> Null) t = ease.Ease(t)
		
		If (time >= target) Then
			t = 1
			time = target
			active = False
		End If
	End Method
	
	Method Start:Void()
		time = 0
		lastTime = Millisecs()
		active = True
	End Method
	
	Method Cancel:Void()
		active = False
	End Method
	
	Method Scale:Float() Property
		Return t
	End Method

End Class

Class Ease
	
	Global QuadIn:EaseFunction = New EaseQuadIn()
	
	Global QuadOut:EaseFunction = New EaseQuadOut()
	
	Global QuadInOut:EaseFunction = New EaseQuadInOut()
	
	Global CubeIn:EaseFunction = New EaseCubeIn()
	
	Global CubeOut:EaseFunction = New EaseCubeOut()
	
	Global CubeInOut:EaseFunction = New EaseCubeInOut()
	
	Global QuartIn:EaseFunction = New EaseQuartIn()
	
	Global QuartOut:EaseFunction = New EaseQuartOut()
	
	Global QuartInOut:EaseFunction = New EaseQuartInOut()
	
	Global QuintIn:EaseFunction = New EaseQuintIn()
	
	Global QuintOut:EaseFunction = New EaseQuintOut()
	
	Global QuintInOut:EaseFunction = New EaseQuintInOut()
	
	Global SineIn:EaseFunction = New EaseSineIn()
	
	Global SineOut:EaseFunction = New EaseSineOut()
	
	Global SineInOut:EaseFunction = New EaseSineInOut()
	
	Global BounceIn:EaseFunction = New EaseBounceIn()
	
	Global BounceOut:EaseFunction = New EaseBounceOut()
	
	Global BounceInOut:EaseFunction = New EaseBounceInOut()
	
	Global CircIn:EaseFunction = New EaseCircIn()
	
	Global CircOut:EaseFunction = New EaseCircOut()
	
	Global CircInOut:EaseFunction = New EaseCircInOut()
	
	Global ExpoIn:EaseFunction = New EaseExpoIn()
	
	Global ExpoOut:EaseFunction = New EaseExpoOut()
	
	Global ExpoInOut:EaseFunction = New EaseExpoInOut()
	
	Global BackIn:EaseFunction = New EaseBackIn()
	
	Global BackOut:EaseFunction = New EaseBackOut()
	
	Global BackInOut:EaseFunction = New EaseBackInOut()

End Class

Interface EaseFunction
	
	Method Ease:Float(t:Float)

End Interface

Private
Const B1:Float = 1 / 2.75
Const B2:Float = 2 / 2.75
Const B3:Float = 1.5 / 2.75
Const B4:Float = 2.5 / 2.75
Const B5:Float = 2.25 / 2.75
Const B6:Float = 2.625 / 2.75

Class EaseQuadIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return t * t
	End Method

End Class

Class EaseQuadOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return -t * (t - 2)
	End Method

End Class

Class EaseQuadInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		If (t <= 0.5) Return t * t * 2
		t -= 1
		Return 1 - t * t * 2
	End Method

End Class

Class EaseCubeIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return t * t * t
	End Method

End Class

Class EaseCubeOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		t -= 1
		Return 1 + t * t * t
	End Method

End Class

Class EaseCubeInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		If (t <= 0.5) Return t * t * t * 4
		t -= 1
		Return 1 + t * t * t * 4
	End Method

End Class

Class EaseQuartIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return t * t * t * t
	End Method

End Class

Class EaseQuartOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		t -= 1
		Return 1 - t * t * t * t
	End Method

End Class

Class EaseQuartInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		If (t <= 0.5) Return t * t * t * t * 8
		t = t * 2 - 2
		Return(1 - t * t * t * t) / 2 + 0.5
	End Method

End Class

Class EaseQuintIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return t * t * t * t * t
	End Method

End Class

Class EaseQuintOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		t -= 1
		Return t * t * t * t * t + 1
	End Method

End Class

Class EaseQuintInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		t *= 2
		If (t < 1) Return(t * t * t * t * t) / 2
		t -= 2
		Return(t * t * t * t * t + 2) / 2
	End Method

End Class

Class EaseSineIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return -Cosr(HALFPI * t) + 1
	End Method

End Class

Class EaseSineOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return Sinr(HALFPI * t)
	End Method

End Class

Class EaseSineInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return -Cosr(PI * t) / 2 + 0.5
	End Method

End Class

Class EaseBounceIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		t = 1 - t
		If (t < B1) Return 1 - 7.5625 * t * t
		If (t < B2) Return 1 - (7.5625 * (t - B3) * (t - B3) + 0.75)
		If (t < B4) Return 1 - (7.5625 * (t - B5) * (t - B5) + 0.9375)
		Return 1 - (7.5625 * (t - B6) * (t - B6) + 0.984375)
	End Method

End Class

Class EaseBounceOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		If (t < B1) Return 7.5625 * t * t
		If (t < B2) Return 7.5625 * (t - B3) * (t - B3) + 0.75
		If (t < B4) Return 7.5625 * (t - B5) * (t - B5) + 0.9375
		Return 7.5625 * (t - B6) * (t - B6) + 0.984375
	End Method

End Class

Class EaseBounceInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		If (t < 0.5) Then
			t = 1 - t * 2
			If (t < B1) Return(1 - 7.5625 * t * t) / 2
			If (t < B2) Return(1 - (7.5625 * (t - B3) * (t - B3) + 0.75)) / 2
			If (t < B4) Return(1 - (7.5625 * (t - B5) * (t - B5) + 0.9375)) / 2
			Return(1 - (7.5625 * (t - B6) * (t - B6) + 0.984375)) / 2
		End If
	
		t = t * 2 - 1
		If (t < B1) Return(7.5625 * t * t) / 2 + 0.5
		If (t < B2) Return(7.5625 * (t - B3) * (t - B3) + 0.75) / 2 + 0.5
		If (t < B4) Return(7.5625 * (t - B5) * (t - B5) + 0.9375) / 2 + 0.5
		Return(7.5625 * (t - B6) * (t - B6) + 0.984375) / 2 + 0.5
	End Method

End Class

Class EaseCircIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return - (Sqrt(1 - t * t) - 1)
	End Method

End Class

Class EaseCircOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return Sqrt(1 - (t - 1) * (t - 1))
	End Method

End Class

Class EaseCircInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		If (t <= 0.5) Return(Sqrt(1 - t * t * 4) - 1) / -2
		Return(Sqrt(1 - (t * 2 - 2) * (t * 2 - 2)) + 1) / 2
	End Method

End Class

Class EaseExpoIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return Pow(2, 10 * (t - 1))
	End Method

End Class

Class EaseExpoOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return -Pow(2, -10 * t) + 1
	End Method

End Class

Class EaseExpoInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		If (t < 0.5) Return Pow(2, 10 * (t * 2 - 1)) / 2
		Return(-Pow(2, -10 * (t * 2 - 1)) + 2) / 2
	End Method

End Class

Class EaseBackIn Implements EaseFunction
	
	Method Ease:Float(t:Float)
		Return t * t * (2.70158 * t - 1.70158)
	End Method

End Class

Class EaseBackOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		t -= 1
		Return 1 - t * t * (-2.70158 * t - 1.70158)
	End Method

End Class

Class EaseBackInOut Implements EaseFunction
	
	Method Ease:Float(t:Float)
		t *= 2
		If (t < 1) Return t * t * (2.70158 * t - 1.70158) / 2
		t -= 2
		Return(1 - t * t * (-2.70158 * t - 1.70158)) / 2 + 0.5
	End Method

End Class