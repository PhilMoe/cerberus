
Import skin

Import graphics

Class Alignment
	Const Center:=0
	Const CenterX:=12
	Const CenterY:=3
	Const Left:=1
	Const Right:=2
	Const Top:=4
	Const Bottom:=8
	Const TopLeft:=Top|Left
	Const TopRight:=Top|Right
	Const BottomLeft:=Bottom|Left
	Const BottomRight:=Bottom|Right
	Const Fill:=15
	Const FillLeft:=13
	Const FillRight:=14
	Const FillTop:=7
	Const FillBottom:=11
End

Class MouseEvent
	Const LeftButtonDown:=1
	Const RightButtonDown:=2
	Const MiddleButtonDown:=3
	Const LeftButtonUp:=4
	Const RightButtonUp:=5
	Const MiddleButtonUp:=6
	Const Movement:=7
End

Class Signal
	Const Clicked:=1
End

Interface IViewListener
	Method OnSignal:Void( signal:Int,view:View )
End

Class View

	Method New()
		Font=Skin.DefaultFont
		Color=Skin.DefaultColor
	End
	
	Method X:Int() Property
		Return _x
	End
	
	Method Y:Int() Property
		Return _y
	End

	Method Width:Int() Property
		Return _width
	End
	
	Method Height:Int() Property
		Return _height
	End
	
	Method Alignment:Void( alignment:Int ) Property
		_alignment=alignment
	End
	
	Method Alignment:Int() Property
		Return _alignment
	End
	
	Method Padding:Void( padding:Int ) Property
		_padding=padding
	End
	
	Method Padding:Int() Property
		Return _padding
	End
	
	Method Font:Void( font:Font ) Property
		_font=font
	End
	
	Method Font:Font() Property
		Return _font
	End
	
	Method Color:Void( color:Float[] ) Property
		_color=color
	End
	
	Method Color:Float[]() Property
		Return _color
	End
	
	Method Text:Void( text:String ) Property
		_text=text
	End
	
	Method Text:String() Property
		Return _text
	End
	
	Method IsEnabled:Void( enabled:Bool ) Property
		_enabled=enabled
	End
	
	Method IsEnabled:Bool() Property
		Return _enabled
	End
	
	'***** Signals *****
	
	Method AddListener:Void( listener:IViewListener )
		_listeners.AddLast listener
	End

	Method RemoveListener:Void( listener:IViewListener )
		_listeners.RemoveEach listener
	End
	
	Method EmitSignal:Void( signal:Int )
		For Local listener:=Eachin _listeners
			listener.OnSignal( signal,Self )
		Next
	End
	
	'***** Events *****
	
	Method OnMouseEvent:Void( event:Int,x:Int,y:Int )
		For Local view:=Eachin _children 
			view.SendMouseEvent( event,x,y )
		Next
	End
	
	Method SendMouseEvent:Void( event:Int,x:Int,y:Int )
		If Not _enabled Return
		x-=_x
		y-=_y
		If x<0 Or x>=_width Return
		If y<0 Or y>=_height Return
		OnMouseEvent( event,x,y )
	End
	
	'***** INTERNAL *****
	
	Method OnMeasure:Void()
	End

	Method OnLayout:Void()
	End
	
	Method OnUpdate:Void()
	End
	
	Method OnRender:Void( gc:GraphicsContext )
	End
	
	Method Skin:Skin() Property
		Return Skin.DefaultSkin
	End
	
	Method Update:Void()
		For Local view:=Eachin _children
			view.Update
		Next
		OnUpdate
	End

	Method Render:Void( gc:GraphicsContext )
	
		If _width<=0 Or _height<=0 Return
		
		Local matrix:=gc.Matrix
		Local left:=matrix[4]+_x,top:=matrix[5]+_y
		Local right:=left+_width,bottom:=top+_height

		Local scissor:=gc.Scissor
'		left=Max( left,scissor[0]+_padding )
'		top=Max( top,scissor[1]+_padding )
'		right=Min( right,scissor[0]+scissor[2]-_padding )
'		bottom=Min( bottom,scissor[1]+scissor[3]-_padding )
		left=Max( left,scissor[0] )
		top=Max( top,scissor[1] )
		right=Min( right,scissor[0]+scissor[2] )
		bottom=Min( bottom,scissor[1]+scissor[3] )
		
		If right<=left Or bottom<=top Return

		'clip
		gc.Scissor=[left,top,right-left,bottom-top]

		'translate
		Local trans:=[matrix[0],matrix[1],matrix[2],matrix[3],matrix[4]+_x,matrix[5]+_y]
		gc.Matrix=trans
		
		gc.Font=Font
		
		If _enabled
			gc.Color=Color
		Else
			gc.Color=[_color[0],_color[1],_color[2],_color[3]*.25]
		End
		
		OnRender gc
		
		For Local view:=Eachin _children
			view.Render gc
		Next

		gc.Matrix=matrix
		
		gc.Scissor=scissor
	End
	
	Method Measure:Void()
		For Local view:=Eachin _children
			view.Measure
		Next
		OnMeasure
	End
	
	Method SetMeasuredSize:Void( width:Int,height:Int )
		_measuredWidth=width
		_measuredHeight=height
	End
	
	Method MeasuredWidth:Int() Property
		Return _measuredWidth
	End
	
	Method MeasuredHeight:Int() Property
		Return _measuredHeight
	End
	
	Method LayoutWidth:Int() Property
		Return _measuredWidth+_padding*2
	End
	
	Method LayoutHeight:Int() Property
		Return _measuredHeight+_padding*2
	End
	
	Method SetLayoutShape:Void( x:Int,y:Int,width:Int,height:Int )
		x+=_padding
		y+=_padding
		width-=_padding*2
		height-=_padding*2
		Local w:=_measuredWidth
		Local h:=_measuredHeight
		Select _alignment&3
		Case 0	'center
			x+=(width-w)/2
		Case 1	'left
		Case 2	'right
			x+=width-w
		Case 3	'fill
			w=width
		End
		Select _alignment Shr 2 & 3
		Case 0	'center
			y+=(height-h)/2
		Case 1	'top
		Case 2	'bottom
			y+=height-h
		Case 3	'fill
			h=height
		End
		SetShape x,y,w,h
		OnLayout
	End
	
	Method SetShape:Void( x:Int,y:Int,width:Int,height:Int )
		_x=x
		_y=y
		_width=width
		_height=height
	End
	
	Method AddChild:Void( view:View )
		_children.AddLast view
	End
	
	Method RemoveChild:Void( view:View )
		_children.RemoveEach view
	End
	
	Private

	Field _alignment:Int
	Field _font:Font
	Field _color:Float[]
	Field _text:String
	Field _x:Int
	Field _y:Int
	Field _width:Int
	Field _height:Int
	Field _measuredWidth:Int
	Field _measuredHeight:Int
	Field _padding:Int=4
	Field _children:=New List<View>
	Field _listeners:=New List<IViewListener>
	Field _enabled:Bool=True
	
End

