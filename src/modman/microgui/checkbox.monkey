
Import view

Class CheckBox Extends View

	Method New( text:String )
		Text=text
	End

	Method IsChecked:Void( checked:Bool ) Property
		_checked=checked
	End
	
	Method IsChecked:Bool() Property
		Return _checked
	End
	
	Private
	
	Field _checked:Bool
	
	Method OnMouseEvent:Void( event:Int,x:Int,y:Int )
		Select event
		Case MouseEvent.LeftButtonDown
			IsChecked=Not IsChecked
		End
	End
	
	Method OnMeasure:Void()
		Skin.MeasureCheckBox Self
	End
	
	Method OnRender:Void( gc:GraphicsContext )
		Skin.RenderCheckBox gc,Self
	End

End
