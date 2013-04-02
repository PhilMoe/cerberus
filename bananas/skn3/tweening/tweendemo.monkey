
Import mojo
Import tween

Class myApp Extends App
    Field tween:Tween
    Field equations:String[] = ["Linear","Back","Bounce","Circ","Cubic","Elastic","Expo","Quad","Quart","Quint","Sine"]
    Field easeTypes:String[] = ["EaseIn","EaseOut","EaseInOut"]

    Field currentEquation:Int
    Field currentEaseType:Int

    Method OnCreate()
        SetUpdateRate(60)

        tween = New Tween(Tween.Linear,100,540,2000)
        tween.Start()
    End

    Method OnUpdate()
        If KeyHit(KEY_1) NextEquation()
        If KeyHit(KEY_2) NextEase()
        If KeyHit(KEY_SPACE)
            tween.Rewind()
            tween.Start()
        Endif
        tween.Update()
    End

    Method OnRender()
        SetColor(0,0,0)
        DrawRect(0,0,640,480)

        SetColor(100,100,100)
        DrawRect(88,198,464,24)

        SetColor(0,0,0)
        DrawRect(89,199,462,22)

        SetColor(255,255,255)
        DrawOval(tween.Value()-10,200,20,20)

        DrawText(EquationString(),5,5)
        DrawText("Press 1 to change tween equation",5,30)
        DrawText("Press 2 to change ease mode",5,45)
        DrawText("Press space to replay current selection",5,60)
    End

    Method EquationString:String()
        If equations[currentEquation] = "Linear"
            Return "Tween.Linear"
        Else
            Return "Tween."+equations[currentEquation]+"."+easeTypes[currentEaseType]
        Endif
    End

    Method NextEquation()
        currentEquation += 1
        If currentEquation >= equations.Length() currentEquation = 0
        SetEquation1()
    End

    Method NextEase()
        currentEaseType += 1
        If currentEaseType >= easeTypes.Length() currentEaseType = 0
        SetEquation1()
    End

    Method SetEquation1()
        Select equations[currentEquation]
            Case "Linear"
                tween.SetEquation(Tween.Linear)
                tween.Rewind()
                tween.Start()
            Case "Back"
                SetEquation2(Tween.Back)
            Case "Bounce"
                SetEquation2(Tween.Bounce)
            Case "Circ"
                SetEquation2(Tween.Circ)
            Case "Cubic"
                SetEquation2(Tween.Cubic)
            Case "Elastic"
                SetEquation2(Tween.Elastic)
            Case "Expo"
                SetEquation2(Tween.Expo)
            Case "Quad"
                SetEquation2(Tween.Quad)
            Case "Quart"
                SetEquation2(Tween.Quart)
            Case "Quint"
                SetEquation2(Tween.Quint)
            Case "Sine"
                SetEquation2(Tween.Sine)
        End
    End

    Method SetEquation2(equation:TweenEquation)
        Select easeTypes[currentEaseType]
            Case "EaseIn"
                tween.SetEquation(equation.EaseIn)
                tween.Rewind()
                tween.Start()
            Case "EaseOut"
                tween.SetEquation(equation.EaseOut)
                tween.Rewind()
                tween.Start()
            Case "EaseInOut"
                tween.SetEquation(equation.EaseInOut)
                tween.Rewind()
                tween.Start()
        End
    End
End

Function Main()
    New myApp
End

