
Import view

Class ImageView Extends View

	Method New( path:String )
		ImagePath=path
	End
	
	Method ImagePath:Void( path:String ) Property
		_path=path
		_image=New Image( path )
	End
	
	Method ImagePath:String() Property
		Return _path
	End
	
	Private
	
	Field _path:String
	Field _image:Image
	
	Method OnMeasure:Void()
		SetMeasuredSize _image.Width,_image.Height
	End
	
	Method OnRender:Void( gc:GraphicsContext )
		gc.Color=[.75,.75,.75,1.0]
		gc.DrawRect( 0,0,Width,Height )
		Local m:=gc.Matrix
		Local xs:=Float( Width )/_image.Width
		Local ys:=Float( Height )/_image.Height
		gc.Matrix=[m[0]*xs,m[1],m[2],m[3]*ys,m[4],m[5]]
		gc.DrawImage _image,0,0
		gc.Matrix=m
	End

End

