
Import view

Class GridView Extends View

	Method New()
		Alignment=.Alignment.Fill
	End
	
	Method New( cols:Int,rows:Int )
		Alignment=.Alignment.Fill
		Columns=cols
		Rows=rows
	End
	
	Method Columns:Void( cols:Int ) Property
		If cols=_cols Return
		_cols=cols
		For Local i:=0 Until _rows
			_views.Set i,_views.Get( i ).Resize( _cols )
		Next
	End
	
	Method Columns:Int() Property
		Return _cols
	End
	
	Method Rows:Void( rows:Int ) Property
		If rows=_rows Return
		_rows=rows
		While _views.Length<_rows
			_views.Push New View[_cols]
		Wend 
		While _views.Length>_rows
			_views.Pop
		Wend
	End
	
	Method Rows:Int() Property
		Return _rows
	End
	
	Method AddRow:Int()
		_rows+=1
		_views.Push New View[_cols]
		Return _rows-1
	End
	
	Method InsertRow:Void( row:Int )
		Error "TODO"
	End
	
	Method RemoveRow:Void( row:Int )
		Error "TODO"
	End
	
	Method SetView:Void( x:Int,y:Int,view:View )
		Local row:=_views.Get( y )
		If row[x] Super.RemoveChild row[x]
		row[x]=view
		If view Super.AddChild view
	End
	
	Method GetView:View( x:Int,y:Int )
		Return _views.Get(y)[x]
	end
	
	Private
	
	Field _rows:Int
	Field _cols:Int
	Field _views:=New Stack<View[]>
	Field _widths:Int[]
	Field _heights:Int[]
	Field _stretchx:Bool
	Field _stretchy:Bool

	Method OnMeasure:Void()
		If Not _rows Or Not _cols
			SetMeasuredSize 0,0
			Return
		Endif
		If _cols<>_widths.Length _widths=New Int[_cols]
		If _rows<>_heights.Length _heights=New Int[_rows]
		For Local i:=0 Until _cols
			_widths[i]=0
		Next
		For Local j:=0 Until _rows
			_heights[j]=0
		Next
		_stretchx=False
		_stretchy=False
		Local w:=0,h:=0
		For Local j:=0 Until _rows
			Local row:=_views.Get( j ),maxh:=0
			For Local i:=0 Until _cols
				Local view:=row[i]
				If Not view Continue
				If (Alignment & 3)=3 And (view.Alignment & 3)=3
					_widths[i]=-1
					_stretchx=True
				Else
					If _widths[i]>=0 _widths[i]=Max( _widths[i],view.LayoutWidth )
				Endif
				If (Alignment Shr 2 & 3)=3 And (view.Alignment Shr 2 & 3)=3
					_heights[j]=-1
					_stretchy=True
				Else
					maxh=Max( maxh,view.LayoutHeight )
				Endif
			Next
			If _heights[j]>=0 _heights[j]=maxh
			h+=maxh
		Next
		For Local t:=Eachin _widths
			w+=t
		Next
		SetMeasuredSize w,h
	End
	
	Method Stretch:Bool( sizes:Int[],size:Int )
		Local sz:=0,n:=0
		For Local i:=0 Until sizes.Length
			If sizes[i]<0
				n+=1
			Else
				sz+=sizes[i]
			Endif
		Next
		Local c:=1
		For Local i:=0 Until sizes.Length
			If sizes[i]<0
				sizes[i]=((size-sz)*c/n)-((size-sz)*(c-1)/n)
				c+=1
			Endif
		Next
	End
	
	Method OnLayout:Void()
	
		Local tx:=0,ty:=0

		If _stretchx
			Stretch _widths,Width
		Else
			tx=(Width-MeasuredWidth)/2
		Endif
		
		If _stretchy
			Stretch _heights,Height
		Else
			ty=(Height-MeasuredHeight)/2
		Endif
	
		Local y:=ty
		For Local j:=0 Until _rows
			Local row:=_views.Get( j )
			Local x:=tx
			For Local i:=0 Until _cols
				Local view:=row[i]
				If view view.SetLayoutShape x,y,_widths[i],_heights[j]
				x+=_widths[i]
			Next
			y+=_heights[j]
		Next
	End
	
	Method OnRender:Void( gc:GraphicsContext )
'		gc.Color=[0.5,0.25,0.125,1.0]
'		gc.DrawRect( 0,0,Width,Height )
	End	

End
