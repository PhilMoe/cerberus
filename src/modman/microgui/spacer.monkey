
Import view

Class Spacer Extends View

	Method New( alignment:Int )
		Alignment=alignment
	End
	
	Private
	
	Method OnRender:Void( gc:GraphicsContext )
		gc.Color=[0.0,0.0,0.0,1.0]
'		gc.DrawRect 0,0,Width,Height
	End
	
End
