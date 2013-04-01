
Import mojo

'--------------------------------------------------------------------------------------------------
'Tween module contents
Const PI := 3.14159265

'---------------
'base classes

Class Tween
    Global Linear:TweenEquationCall = New TweenEquationCallLinear

    Global Back:TweenEquation = New TweenEquationBack
    Global Bounce:TweenEquation = New TweenEquationBounce
    Global Circ:TweenEquation = New TweenEquationCirc
    Global Cubic:TweenEquation = New TweenEquationCubic
    Global Elastic:TweenEquation = New TweenEquationElastic
    Global Expo:TweenEquation = New TweenEquationExpo
    Global Quad:TweenEquation = New TweenEquationQuad
    Global Quart:TweenEquation = New TweenEquationQuart
    Global Quint:TweenEquation = New TweenEquationQuint
    Global Sine:TweenEquation = New TweenEquationSine

    Field equation:TweenEquationCall

    Field start:Float
    Field change:Float
    Field current:Float

    Field isActive := False
    Field isLooping := False
    Field isYoYo := False

    Field duration:Int
    Field loopCount:Int
    Field timeStart:Int
    Field timeCurrent:Int
    Field timePrevious:Int

    ' --- constructors ---
    Method New(equation:TweenEquationCall,startValue:Float,endValue:Float,duration:Int)
        SetEquation(equation)
        SetDuration(duration)
        SetValue(startValue,endValue)
    End

    ' --- private methods ---
    Private
    Method UpdateValue()
        current = equation.Call(timeCurrent,start,change,duration)
    End
    Public

    ' --- value methods ---
    Method Value:Float()
        Return current
    End

    ' --- setup methods methods ---
    Method SetEquation(equation:TweenEquationCall)
        Self.equation = equation
    End

    Method SetDuration(duration:Int)
        Self.duration = duration
    End

    Method SetValue(startValue:Float,endValue:Float)
        start = startValue
        change = endValue - startValue
    End

    Method SetYoYo(flag:Bool)
        isYoYo = flag
    End

    Method SetLooping(flag:Bool)
        isLooping = flag
    End

    ' --- control methods ---
    Method Start()
        ' --- this will re-start the tween ---
        Rewind()
        isActive = True
        loopCount = 0
    End

    Method Stop()
        ' --- stop the tween from updating ---
        isActive = False
    End

    Method Resume()
        ' --- resume the tween from a paused state ---
        isActive = True

        'update the start timer!

    End

    Method Rewind()
        ' --- put the tween at the start of its tween ---
        timeCurrent = 0
        timeStart = Millisecs()
        UpdateValue()
    End

    Method FastForward()
        ' --- set the tween at the end ---
        timeCurrent = duration
        UpdateValue()
        Stop()
    End

    Method ContinueTo(endValue:Float,duration:Int=0)
        ' --- change the end target for teh value
        start = current
        SetValue(start,endValue)

        If isActive
            'isActive so need to continue operation
            If duration = 0
                'no duration specified so use previous duration!
                Self.duration = duration - timeCurrent
            Else
                'override duration
                Self.duration = duration
            Endif

            timeStart = Millisecs()
            timeCurrent = 0

            If duration <= 0
                duration = 0
                Stop()
            Endif
        Else
            If duration > 0 SetDuration(duration)
            Start()
        Endif
    End

    Method YoYo()
        ContinueTo(start)
    End

    ' --- class methods ---
    Method Update()
        If isActive
            'update the current time!
            timePrevious = timeCurrent
            Local time := Millisecs() - timeStart

            If time > duration
                'time is beyond length of tween
                If isLooping Or isYoYo
                    'increase the loop count
                    loopCount += 1

                    'look at yoyoing the values
                    If isYoYo
                        timeCurrent = duration
                        current = start + change

                        If isLooping Or loopCount <= 1
                            ContinueTo(start,duration)
                        Else
                            Stop()
                        Endif
                    Else
                        'wrap around the tween to the start
                        timeCurrent = 0'
                        timeStart = Millisecs()
                    Endif
                Else
                    'set the tween to the end
                    timeCurrent = duration
                    current = start + change
                    Stop()
                Endif
            Else If time < 0
                'time is before the start of the tween
                timeCurrent = 0
                timeStart = Millisecs()
                UpdateValue()
            Else
                'time is within the tween
                timeCurrent = time
                UpdateValue()
            Endif
        Endif
    End
End

Class TweenEquation
    ' --- really just a grouping class for user access ---
    Field EaseIn:TweenEquationCall
    Field EaseOut:TweenEquationCall
    Field EaseInOut:TweenEquationCall
End

Class TweenEquationCall
    ' --- base class to provide a call function pointer work around ---
    Method Call:Float(t:Float,b:Float,c:Float,d:Float) Abstract
End



'---------------
'Linear
Class TweenEquationCallLinear Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        Return c*t/d + b
    End
End

'---------------
'Sine
Class TweenEquationSine Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallSineEaseIn
        EaseOut = New TweenEquationCallSineEaseOut
        EaseInOut = New TweenEquationCallSineEaseInOut
    End
End

Class TweenEquationCallSineEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        'return -c * Cos(t / d * (PI / 2)) + c + b
        Return -c * Cos((t / d * (PI / 2)) * 57.2957795) + c + b
    End
End

Class TweenEquationCallSineEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        Return c * Sin((t/d * (PI/2)) * 57.2957795) + b
    End
End

Class TweenEquationCallSineEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        Return -c/2 * (Cos((PI*t/d) * 57.2957795) - 1) + b
    End
End



'---------------
'Back
Class TweenEquationBack Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallBackEaseIn
        EaseOut = New TweenEquationCallBackEaseOut
        EaseInOut = New TweenEquationCallBackEaseInOut
    End
End

Class TweenEquationCallBackEaseIn Extends TweenEquationCall
    Field s := 1.70158
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
		Return c * t * t * ((s + 1) * t - s) + b
    End
End

Class TweenEquationCallBackEaseOut Extends TweenEquationCall
    Field s := 1.70158
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t = t / d - 1
		Return c * (t * t * ((s + 1) * t + s) + 1) + b
    End
End

Class TweenEquationCallBackEaseInOut Extends TweenEquationCall
    Field s := 1.70158
    Field s2:Float
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        s2 = s
        t /= d / 2
        s2 *= 1.525
		If t < 1
            Return c / 2 * (t * t *((s2+1) * t - s2)) + b
        Endif
		t -= 2
		Return c / 2 * (t * t * ((s2 + 1) * t + s2) + 2) + b
    End
End



'---------------
'Elastic
Class TweenEquationElastic Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallElasticEaseIn
        EaseOut = New TweenEquationCallElasticEaseOut
        EaseInOut = New TweenEquationCallElasticEaseInOut
    End
End

Class TweenEquationCallElasticEaseIn Extends TweenEquationCall
    Field p:Float
    Field a:Float
    Field s:Float

    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
		If t = 0 Return b
		t /= d
        If t = 1 Return b+c
        p = d *  0.3
		a = c
        s = p / 4
        t -= 1
		Return -(a * Pow(2,10 * (t)) * Sin(((t * d - s) * (2 * PI) / p) * 57.2957795)) + b
    End
End

Class TweenEquationCallElasticEaseOut Extends TweenEquationCall
    Field p:Float
    Field a:Float
    Field s:Float

    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
		If t = 0 Return b
		t /= d
        If t = 1 Return b+c
        p = d * 0.3
		a = c
        s = p / 4
		Return (a * Pow(2,-10 * t) * Sin(((t * d - s) * (2 * PI) / p) * 57.2957795) + c + b)
    End
End

Class TweenEquationCallElasticEaseInOut Extends TweenEquationCall
    Field p:Float
    Field a:Float
    Field s:Float

    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
		If t = 0 Return b
		t /= d / 2
        If t = 2 Return b+c
        p = d * (0.3 * 1.5)
		a = c
        s = p / 4
		If t < 1
            t -= 1
            Return -0.5 * (a * Pow(2,10 * t) * Sin(((t * d - s) * (2 * PI) / p) * 57.2957795)) + b
        Endif
        t -= 1
		Return a * Pow(2,-10 * t) * Sin(((t * d - s) * (2 * PI) / p) * 57.2957795) * 0.5 + c + b
    End
End



'---------------
'Bounce
Class TweenEquationBounce Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallBounceEaseIn
        EaseOut = New TweenEquationCallBounceEaseOut
        EaseInOut = New TweenEquationCallBounceEaseInOut
    End
End

Class TweenEquationCallBounceEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t = (d-t) / d
		If t < 0.3636363
			Return c - (c * (7.5625 * t * t)) + b
		Else If t < 0.7272727
            t -= 0.5454545
			Return c - (c * (7.5625 * t * t + 0.75)) + b
		Else If t < 0.9090909
            t -= 0.8181818
			Return c - (c * (7.5625 * t * t + 0.9375)) + b
		Else
            t -= 0.9636363
			Return c - (c * (7.5625 * t * t + 0.984375)) + b
        Endif
    End
End

Class TweenEquationCallBounceEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
		If t < 0.3636363
			Return c *(7.5625 * t * t) + b
		Else If t < 0.7272727
            t -= 0.5454545
			Return c * (7.5625 * t * t + 0.75) + b
		Else If t < 0.9090909
            t -= 0.8181818
			Return c * (7.5625 * t * t + 0.9375) + b
		Else
            t -= 0.9636363
			Return c * (7.5625 * t * t + 0.984375) + b
        Endif
    End
End

Class TweenEquationCallBounceEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
		If t < d/2
            t = (d - t * 2) / d
    		If t < 0.3636363
    			Return (c - (c * (7.5625 * t * t))) * 0.5 + b
    		Else If t < 0.7272727
                t -= 0.5454545
    			Return (c - (c * (7.5625 * t * t + 0.75))) * 0.5 + b
    		Else If t < 0.9090909
                t -= 0.8181818
    			Return (c - (c * (7.5625 * t * t + 0.9375))) * 0.5 + b
    		Else
                t -= 0.9636363
    			Return (c - (c * (7.5625 * t * t + 0.984375))) * 0.5 + b
            Endif
        Else
            t = (t * 2 - d) / d
    		If t < 0.3636363
    			Return (c *(7.5625 * t * t)) * 0.5 + c * 0.5 + b
    		Else If t < 0.7272727
                t -= 0.5454545
    			Return (c * (7.5625 * t * t + 0.75)) * 0.5 + c * 0.5 + b
    		Else If t < 0.9090909
                t -= 0.8181818
    			Return (c * (7.5625 * t * t + 0.9375)) * 0.5 + c * 0.5 + b
    		Else
                t -= 0.9636363
    			Return (c * (7.5625 * t * t + 0.984375)) * 0.5 + c * 0.5 + b
            Endif
        Endif
    End
End



'---------------
'Circ
Class TweenEquationCirc Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallCircEaseIn
        EaseOut = New TweenEquationCallCircEaseOut
        EaseInOut = New TweenEquationCallCircEaseInOut
    End
End

Class TweenEquationCallCircEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
        Return -c * (Sqrt(1 - t * t) - 1) + b
    End
End

Class TweenEquationCallCircEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t = t / d - 1
        Return c * Sqrt(1 - t*t) + b
    End
End

Class TweenEquationCallCircEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d / 2
		If t < 1 Return -c / 2 * (Sqrt(1 - t * t) - 1) + b
		t -= 2
		Return c / 2 * (Sqrt(1 - t * t) + 1) + b
    End
End



'---------------
'Cubic
Class TweenEquationCubic Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallCubicEaseIn
        EaseOut = New TweenEquationCallCubicEaseOut
        EaseInOut = New TweenEquationCallCubicEaseInOut
    End
End

Class TweenEquationCallCubicEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
        Return c * t * t * t + b
    End
End

Class TweenEquationCallCubicEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t = t / d - 1
        Return c * (t * t * t + 1) + b
    End
End

Class TweenEquationCallCubicEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d / 2
		If t < 1 Return c / 2 * t * t * t + b
		t -= 2
		Return c / 2 *(t * t * t + 2) + b
    End
End



'---------------
'Expo
Class TweenEquationExpo Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallExpoEaseIn
        EaseOut = New TweenEquationCallExpoEaseOut
        EaseInOut = New TweenEquationCallExpoEaseInOut
    End
End

Class TweenEquationCallExpoEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        If t = 0 Return b
        Return c * Pow(2,10 * (t / d - 1)) + b
    End
End

Class TweenEquationCallExpoEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        If t = d Return b + c
        Return c * (-Pow(2,-10 * t / d) + 1) + b
    End
End

Class TweenEquationCallExpoEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
		If t =0 Return b
		If t = d Return b + c
		t /= d / 2
		If t < 1 Return c / 2 * Pow(2,10 * (t - 1)) + b
		t -= 1
		Return c / 2 * (-Pow(2,-10 * t) + 2) + b
    End
End



'---------------
'Quad
Class TweenEquationQuad Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallQuadEaseIn
        EaseOut = New TweenEquationCallQuadEaseOut
        EaseInOut = New TweenEquationCallQuadEaseInOut
    End
End

Class TweenEquationCallQuadEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
        Return c * t * t + b
    End
End

Class TweenEquationCallQuadEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
        Return -c * t * (t - 2) + b
    End
End

Class TweenEquationCallQuadEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d / 2
		If t < 1 Return c / 2 * t * t + b
		t -= 1
		Return -c / 2 * (t * (t - 2) - 1) + b
    End
End



'---------------
'Quart
Class TweenEquationQuart Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallQuartEaseIn
        EaseOut = New TweenEquationCallQuartEaseOut
        EaseInOut = New TweenEquationCallQuartEaseInOut
    End
End

Class TweenEquationCallQuartEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
        Return c * t * t * t * t + b
    End
End

Class TweenEquationCallQuartEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t = t / d-1
        Return -c * (t * t * t * t - 1) + b
    End
End

Class TweenEquationCallQuartEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d / 2
		If t < 1 Return c / 2 * t * t * t * t + b
        t -= 2
		Return -c / 2 * (t * t * t * t - 2) + b
    End
End



'---------------
'Quint
Class TweenEquationQuint Extends TweenEquation
    Method New()
        EaseIn = New TweenEquationCallQuintEaseIn
        EaseOut = New TweenEquationCallQuintEaseOut
        EaseInOut = New TweenEquationCallQuintEaseInOut
    End
End

Class TweenEquationCallQuintEaseIn Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d
        Return c * t * t * t * t * t + b
    End
End

Class TweenEquationCallQuintEaseOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t = t / d - 1
        Return c * (t * t * t * t * t + 1) + b
    End
End

Class TweenEquationCallQuintEaseInOut Extends TweenEquationCall
    Method Call:Float(t:Float,b:Float,c:Float,d:Float)
        t /= d / 2
		If t < 1 Return c / 2 * t * t * t * t * t + b
        t -= 2
		Return c / 2 * (t * t * t * t * t + 2) + b
    End
End
