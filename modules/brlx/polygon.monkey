
Class Polygon

	Method New( vertices:Vertex[],material:Material )
	End
	
	Method New( plane:Plane,size# )
	End
	
	Method Valid:Bool()
	End
	
	Method Plane:Plane()
	End
	
	Method Normal:Normal()
	End
	
	Method Split:Int( plane:Plane,bits:Polygon[] )
	End

End
