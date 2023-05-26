
Class Vec3 Final

	'Read only!
	'
	Field x#,y#,z#

	Method New()
	End
	
	Method New( vx#,vy#,vz# )
		x=vx;y=vy;z=vz
	End
	
	Method New( v:Vec3 )
		x=v.x;y=v.y;z=v.z
	End
	
	Method Set:Void( vx#,vy#,vz# )
		x=vx;y=vy;z=vz
	End
	
	Method Set:Void( v:Vec3 )
		x=v.x;y=v.y;z=v.z
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
	
	Method Plus:Vec3( vx#,vy#,vz# )
		Return Tmp( x+vx,y+vy,z+vz )
	End
	
	Method Plus:Vec3( v:Vec3 )
		Return Tmp( x+v.x,y+v.y,z+v.z )
	End
	
	Method Minus:Vec3( n# )
		Return Tmp( x-n,y-n,z-n )
	End
	
	Method Minus:Vec3( vx#,vy#,vz# )
		Return Tmp( x-vx,y-vy,z-vz )
	End
	
	Method Minus:Vec3( v:Vec3 )
		Return Tmp( x-v.x,y-v.y,z-v.z )
	End
	
	Method Times:Vec3( n# )
		Return Tmp( x*n,y*n,z*n )
	End
	
	Method Times:Vec3( vx#,vy#,vz# )
		Return Tmp( x*vx,y*vy,z*vz )
	End
	
	Method Times:Vec3( v:Vec3 )
		Return Tmp( x*v.x,y*v.y,z*v.z )
	End
	
	Method DividedBy:Vec3( n# )
		Return Tmp( x/n,y/n,z/n )
	End
	
	Method DividedBy:Vec3( vx#,vy#,vz# )
		Return Tmp( x/vx,y/vy,z/vz )
	End
	
	Method DividedBy:Vec3( v:Vec3 )
		Return Tmp( x/v.x,y/v.y,z/v.z )
	End
	
	Method Cross:Vec3( vx#,vy#,vz# )
		Return Tmp( y*vz-z*vy,z*vx-x*vz,x*vy-y*vx )
	End
	
	Method Cross:Vec3( v:Vec3 )
		Return Tmp( y*v.z-z*v.y,z*v.x-x*v.z,x*v.y-y*v.x )
	End
	
	Method Dot#( vx#,vy#,vz# )
		Return x*vx+y*vy+z*vz
	End
	
	Method Dot#( v:Vec3 )
		Return x*v.x+y*v.y+z*v.z
	End

	Method Length#()
		Return Sqrt( x*x+y*y+z*z )
	End
	
	Method Normalize:Vec3()
		Local l:=Length()
		Return Tmp( x/l,y/l,z/l )
	End
	
	Function Tmp:Vec3()
		Local t:=AllocTmp()
		t.x=0;t.y=0;t.z=0
		Return t
	End
	
	Function Tmp:Vec3( vx#,vy#,vz# )
		Local t:=AllocTmp()
		t.x=vx;t.y=vy;t.z=vz
		Return t
	End
	
	Function Tmp:Vec3( v:Vec3 )
		Local t:=AllocTmp()
		t.x=v.x;t.y=v.y;t.z=v.z
		Return t
	End
	
	'Returns unitinitalized tmp!
	'
	Function AllocTmp:Vec3()
		_tmp+=1
		If _tmp=_tmps.Length
			_tmps=_tmps.Resize( _tmps.Length*2 )
			For Local i=_tmp Until _tmps.Length
				_tmps[i]=New Vec3
			Next
		Endif
		Return _tmps[_tmp]
	End

	'Internal use...
	'
	Function GetTmps()
		Return _tmp
	End
	
	'Internal use...
	'
	Function SetTmps( n )
		_tmp=n
	End
	
	Private	
	
	Global _tmp,_tmps:=InitTmps()
	
	Function InitTmps:Vec3[]()
		Local tmps:=New Vec3[256]
		For Local i=1 Until tmps.Length
			tmps[i]=New Vec3
		Next
		Return tmps
	End

End
