
Import pool

Class Vec3

	Field x#,y#,z#
	
	Method New()
	End
	
	Method New( n# )
		x=n;y=n;z=n
	End
	
	Method New( v:Vec3 )
		x=v.x;y=v.y;z=v.z
	End
	
	Method New( vx#,vy#,vz# )
		x=vx;y=vy;z=vz
	End
	
	Method Set:Void( n# )
		x=n;y=n;z=n
	End
	
	Method Set:Void( v:Vec3 )
		x=v.x;y=v.y;z=v.z
	End
	
	Method Set:Void( vx#,vy#,vz# )
		x=vx;y=vy;z=vz
	End
	
	Method ToString$()
		Return "Vec3("+x+","+y+","+z+")"
	End
	
	Method ToArray#[]()
		Return [ x,y,z ]
	End
	
	Method ToArray:Void( v#[] )
		v[0]=x;v[1]=y;v[2]=z
	End
	
	Method Negate:Vec3()
		Return Tmp( -x,-y,-z )
	End
	
	Method Plus:Vec3( n# )
		Return Tmp( x+n,y+n,z+n )
	End
	
	Method Plus:Vec3( v:Vec3 )
		Return Tmp( x+v.x,y+v.y,z+v.z )
	End
	
	Method Plus:Vec3( vx#,vy#,vz# )
		Return Tmp( x+vx,y+vy,z+vz )
	End
	
	Method Minus:Vec3( n# )
		Return Tmp( x-n,y-n,z-n )
	End
	
	Method Minus:Vec3( v:Vec3 )
		Return Tmp( x-v.x,y-v.y,z-v.z )
	End
	
	Method Minus:Vec3( vx#,vy#,vz# )
		Return Tmp( x-vx,y-vy,z-vz )
	End
	
	Method Times:Vec3( n# )
		Return Tmp( x*n,y*n,z*n )
	End
	
	Method Times:Vec3( v:Vec3 )
		Return Tmp( x*v.x,y*v.y,z*v.z )
	End
	
	Method Times:Vec3( vx#,vy#,vz# )
		Return Tmp( x*vx,y*vy,z*vz )
	End
	
	Method DividedBy:Vec3( n# )
		Return Tmp( x/n,y/n,z/n )
	End
	
	Method DividedBy:Vec3( v:Vec3 )
		Return Tmp( x/v.x,y/v.y,z/v.z )
	End
	
	Method DividedBy:Vec3( vx#,vy#,vz# )
		Return Tmp( x/vx,y/vy,z/vz )
	End
	
	Method Cross:Vec3( v:Vec3 )
		Return Tmp( y*v.z-z*v.y,z*v.x-x*v.z,x*v.y-y*v.x )
	End
	
	Method Cross:Vec3( vx#,vy#,vz# )
		Return Tmp( y*vz-z*vy,z*vx-x*vz,x*vy-y*vx )
	End
	
	Method Dot:Float( v:Vec3 )
		Return x*v.x+y*v.y+z*v.z
	End

	Method Dot:Float( vx#,vy#,vz# )
		Return x*vx+y*vy+z*vz
	End
	
	Method Length:Float()
		Return Sqrt( x*x+y*y+z*z )
	End
	
	Method Normalize:Vec3()
		Local l:=Length()
		Return Tmp( x/l,y/l,z/l )
	End
	
	Function Tmp:Vec3()
		Local t:=Pool<Vec3>.AllocTmp()
		t.x=0;t.y=0;t.z=0
		Return t
	End
	
	Function Tmp:Vec3( n# )
		Local t:=Pool<Vec3>.AllocTmp()
		t.x=n;t.y=n;t.z=n
		Return t
	End
	
	Function Tmp:Vec3( v:Vec3 )
		Local t:=Pool<Vec3>.AllocTmp()
		t.x=v.x;t.y=v.y;t.z=v.z
		Return t
	End

	Function Tmp:Vec3( vx#,vy#,vz# )
		Local t:=Pool<Vec3>.AllocTmp()
		t.x=vx;t.y=vy;t.z=vz
		Return t
	End
	
End
