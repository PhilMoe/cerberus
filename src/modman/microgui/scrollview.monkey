
Import view

Import mojo.input

Class ScrollView Extends View

	Method New()
		Alignment=.Alignment.Fill
	End
	
	Method New( content:View )
		Alignment=.Alignment.Fill
		Content=content
	End

	Method Content:Void( content:View ) Property
		If _content Super.RemoveChild _content
		_content=content
		If _content Super.AddChild _content
		_tx=0
		_ty=0
	End
	
	Method Content:View() Property
		Return _content
	End
	
	Private
	
	Field _content:View
	Field _tx:Int,_ty:Int
	
	Method OnMeasure:Void()
		If _content SetMeasuredSize _content.LayoutWidth,_content.LayoutHeight
	End

	Method OnLayout:Void()
		If _content _content.SetLayoutShape _tx,_ty,Width,Height	'_content.LayoutWidth,_content.LayoutHeight
	End
	
	Method OnUpdate:Void()
		If KeyHit( KEY_LEFT )
			_tx+=5
		Else If KeyHit( KEY_RIGHT )
			_tx-=5
		End
		If KeyHit( KEY_UP )
			_ty+=5
		Else If KeyHit( KEY_DOWN )
			_ty-=5
		Endif
	End
	
	Method OnRender:Void( gc:GraphicsContext )
		gc.Color=[0.0,0.5,1.0,1.0]
'		gc.DrawRect( 0,0,Width,Height )
	End
		
End

