
Class Plane Extends Value Final

	Field x#,y#,z#,w#
	
	Method New( x#,y#,z#,w# )
		Self.x=x ; Self.y=y ; Self.z=z ; Self.w=w
	End
	
	Method New( p:Vec3,n:Vec3 )
	End

	Method Normal:Vec3()
	End
	
	Method Offset:Float()
	End
	
End
