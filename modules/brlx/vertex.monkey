
Class Vertex

	Field x#,y#,z#
	Field nx#,ny#,nz#
	Field tx#,ty#,tz#,tw#
	Field s0#,t0#,p0#
	Field s1#,t1#,p1#
	Field w0#,w1#,w2#,w3#
	Field boneIndices
	
	Method New( x#,y#,z# )
	End
	
	Method Copy:Vertex()
	End
	
	Method SetPosition:Void( v:Vec3 )
		x=v.x;y=v.y;z=v.z
	End
	
	Method Postion:Vec3()
		Return Vec3.Tmp(x,y,z)
	End
	
	Method SetNormal:Void( v:Vec3 )
		nx=v.x;ny=v.y;nz=v.z
	End
	
	Method Normal:Vec3()
		Return Vec3.Tmp(nx,ny,nz)
	End
	
	Method SetTangent:Void( v:Vec4 )
	End
	
	Method Tangent:Vec3()	
		Return Vec3.Tmp(tx,ty,tz,tw)
	End
	
	Method SetTexCoords:Void( index,v:Vec3 )
	End
	
	Method TexCoords:Vec3( index )
		Select index
		Case 0 Return Vec4.Tmp(s0,t0,p0)
		Case 1 Return Vec4.Tmp(s1,t1,p1)
		End
		Return Vec4.Tmp()
	End
	
	Method SetWeight:Void( index,weight# )
	End
	
	Method Weight:Float( index )
		Select index
		Case 0 Return w0
		Case 1 Return w1
		Case 2 Return w2
		Case 3 Return w3
		End
		Return 0
	End
	
	Method Weights:Vec4()
		Return Vec4(w0,w1,w2,w3)
	End
	
	Method BoneIndex:Int( index )
		Return (indices Shr (index Shl 3)) & 255
	End
	
	Method Blend:Vertex( v:Vertex,alpha# )
		Local beta#=1-alpha
		x=x*alpha+v.x*beta ; y=y*alpha+v.y*beta ; z=z*alpha+v.z*beta
	End
	
End
