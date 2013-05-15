
Import view

Class Label Extends View

	Method New( text:String )
		Text=text
	End
	
	Private
	
	Method OnMouseEvent:Void( event:Int,x:Int,y:Int )
		Select event
		Case MouseEvent.LeftButtonDown
			EmitSignal( Signal.Clicked )
		End
	End
	
	Method OnMeasure:Void()
		Skin.MeasureLabel Self
	End

	Method OnRender:Void( gc:GraphicsContext )
		Skin.RenderLabel gc,Self
	End
	
End
