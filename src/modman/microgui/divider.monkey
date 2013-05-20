
Import view

Class Divider Extends View

	Method New( alignment:Int )
		Alignment=alignment
	End
	
	Private
	
	Method OnMeasure:Void()
		Skin.MeasureDivider Self
	End
	
	Method OnRender:Void( gc:GraphicsContext )
		Skin.RenderDivider gc,Self
	End

End
