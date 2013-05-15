
Import view

Class Window Extends View

	Method New()
	End
	
	Method New( content:View )
		Content=content
	End

	Method Content:Void( content:View ) Property
		If _content RemoveChild _content
		_content=content
		If _content AddChild _content
	End
	
	Method Content:View() Property
		Return _content
	End
	
	Method SetShape:Void( x:Int,y:Int,width:Int,height:Int )
		Super.SetShape x,y,width,height
		If _content
			_content.Measure
			_content.SetLayoutShape 0,0,width,height
		End
	End
	
	Private
	
	Field _content:View
	
	Method OnRender:Void( gc:GraphicsContext )
		gc.Color=[.95,.95,.95,1.0]
		gc.DrawRect( 0,0,Width,Height )
	End
	
End

