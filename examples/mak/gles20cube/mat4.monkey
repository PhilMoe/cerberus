
Import vec3

Class Mat4 Final

	'Read only!
	'
	Field ix#,iy#,iz#,iw#
	Field jx#,jy#,jz#,jw#
	Field kx#,ky#,kz#,kw#
	Field tx#,ty#,tz#,tw#
	
	Method New()
		ix=1;jy=1;kz=1;tw=1
	End
	
	Method New( m#[] )
		ix=m[0];iy=m[1];iz=m[2];iw=m[3]
		jx=m[4];jy=m[5];jz=m[6];jw=m[7]
		kx=m[8];ky=m[9];kz=m[10];kw=m[11]
		tx=m[12];ty=m[13];tz=m[14];tw=m[15]
	End
	
	Method New( m:Mat4 )
		ix=m.ix;iy=m.iy;iz=m.iz;iw=m.iw
		jx=m.jx;jy=m.jy;jz=m.jz;jw=m.jw
		kx=m.kx;ky=m.ky;kz=m.kz;kw=m.kw
		tx=m.tx;ty=m.ty;tz=m.tz;tw=m.tw
	End
	
	Method Set:Void( m#[] )
		ix=m[0];iy=m[1];iz=m[2];iw=m[3]
		jx=m[4];jy=m[5];jz=m[6];jw=m[7]
		kx=m[8];ky=m[9];kz=m[10];kw=m[11]
		tx=m[12];ty=m[13];tz=m[14];tw=m[15]
	End
	
	Method Set:Void( m:Mat4 )
		ix=m.ix;iy=m.iy;iz=m.iz;iw=m.iw
		jx=m.jx;jy=m.jy;jz=m.jz;jw=m.jw
		kx=m.kx;ky=m.ky;kz=m.kz;kw=m.kw
		tx=m.tx;ty=m.ty;tz=m.tz;tw=m.tw
	End
	
	Method ToString$()
		Return "Mat4("+ix+","+iy+","+iz+","+iw+","+jx+","+jy+","+jz+","+jk+","+kx+","+ky+","+kz+","+kw+","+tx+","+ty+","+tz+","+tw+")"
	End
		
	Method ToArray#[]()
		Return [ ix,iy,iz,iw, jx,jy,jz,jw, kx,ky,kz,kw, tx,ty,tz,tw ]
	End
	
	Method ToArray:Void( m#[] )
		m[0]=ix;m[1]=iy;m[2]=iz;m[3]=iw
		m[4]=jx;m[5]=jy;m[6]=jz;m[7]=jw
		m[8]=kx;m[9]=ky;m[10]=kz;m[11]=kw
		m[12]=tx;m[13]=ty;m[14]=tz;m[15]=tw
	End
	
	Method Inverse:Mat4()
		If iw=0 And jw=0 And kw=0 And tw=1
			'faster affine inverse only for now...
			Local t:=AllocTmp()
			Local c#=1/(ix*(jy*kz-jz*ky)-iy*(jx*kz-jz*kx)+iz*(jx*ky-jy*kx))
			t.ix= c * ( jy*kz - jz*ky )
			t.iy=-c * ( iy*kz - iz*ky )
			t.iz= c * ( iy*jz - iz*jy )
			t.jx=-c * ( jx*kz - jz*kx )
			t.jy= c * ( ix*kz - iz*kx )
			t.jz=-c * ( ix*jz - iz*jx )
			t.kx= c * ( jx*ky - jy*kx )
			t.ky=-c * ( ix*ky - iy*kx )
			t.kz= c * ( ix*jy - iy*jx )
			t.tx=-( tx*t.ix + ty*t.jx + tz*t.kx )
			t.ty=-( tx*t.iy + ty*t.jy + tz*t.ky )
			t.tz=-( tx*t.iz + ty*t.jz + tz*t.kz )
			t.tw=1
			Return t
		Endif
		Error "Can't invert matrix"
	End
	
	Method Transpose:Mat4()
		Local t:=AllocTmp()
		t.ix=ix;t.iy=jx;t.iz=kx;t.iw=tx
		t.jx=iy;t.jy=jy;t.jz=ky;t.jw=ty
		t.kx=iz;t.ky=jz;t.kz=kz;t.kw=tz
		t.tx=iw;t.ty=jw;t.tz=kw;t.tw=tw
		Return t
	End
	
	Method Times:Vec3( v:Vec3 )
		Local t:=Vec3.AllocTmp()
		t.x=ix*v.x+jx*v.y+kx*v.z+tx
		t.y=iy*v.x+jy*v.y+ky*v.z+ty
		t.z=iz*v.x+jz*v.y+kz*v.z+tz
		Return t
	End

	Method Times:Mat4( m:Mat4 )
		Local t:=AllocTmp()
		t.ix= ix*m.ix + jx*m.iy + kx*m.iz + tx*m.iw
		t.iy= iy*m.ix + jy*m.iy + ky*m.iz + ty*m.iw
		t.iz= iz*m.ix + jz*m.iy + kz*m.iz + tz*m.iw
		t.iw= iw*m.ix + jw*m.iy + kw*m.iz + tw*m.iw
		t.jx= ix*m.jx + jx*m.jy + kx*m.jz + tx*m.jw
		t.jy= iy*m.jx + jy*m.jy + ky*m.jz + ty*m.jw
		t.jz= iz*m.jx + jz*m.jy + kz*m.jz + tz*m.jw
		t.jw= iw*m.jx + jw*m.jy + kw*m.jz + tw*m.jw
		t.kx= ix*m.kx + jx*m.ky + kx*m.kz + tx*m.kw
		t.ky= iy*m.kx + jy*m.ky + ky*m.kz + ty*m.kw
		t.kz= iz*m.kx + jz*m.ky + kz*m.kz + tz*m.kw
		t.kw= iw*m.kx + jw*m.ky + kw*m.kz + tw*m.kw
		t.tx= ix*m.tx + jx*m.ty + kx*m.tz + tx*m.tw
		t.ty= iy*m.tx + jy*m.ty + ky*m.tz + ty*m.tw
		t.tz= iz*m.tx + jz*m.ty + kz*m.tz + tz*m.tw
		t.tw= iw*m.tx + jw*m.ty + kw*m.tz + tw*m.tw
		Return t	
	End
	
	Function Tmp:Mat4()
		Local t:=AllocTmp()
		t.ix=1;t.iy=0;t.iz=0;t.iw=0
		t.jx=0;t.jy=1;t.jz=0;t.jw=0
		t.kx=0;t.ky=0;t.kz=1;t.kw=0
		t.tx=0;t.ty=0;t.tz=0;t.tw=1
		Return t
	End
	
	Function Tmp:Mat4( m#[] )
		Local t:=AllocTmp()
		t.ix=m[0];t.iy=m[1];t.iz=m[2];t.iw=m[3]
		t.jx=m[4];t.jy=m[5];t.jz=m[6];t.jw=m[7]
		t.kx=m[8];t.ky=m[9];t.kz=m[10];t.kw=m[11]
		t.tx=m[12];t.ty=m[13];t.tz=m[14];t.tw=m[15]
		Return t
	End
	
	Function Tmp:Mat4( m:Mat4 )
		Local t:=AllocTmp()
		t.ix=m.ix;t.iy=m.iy;t.iz=m.iz;t.iw=m.iw
		t.jx=m.jx;t.jy=m.jy;t.jz=m.jz;t.jw=m.jw
		t.kx=m.kx;t.ky=m.ky;t.kz=m.kz;t.kw=m.kw
		t.tx=m.tx;t.ty=m.ty;t.tz=m.tz;t.tw=m.tw
		Return t
	End

	'Returns unitinitalized tmp!
	'
	Function AllocTmp:Mat4()
		_tmp+=1
		If _tmp=_tmps.Length
			_tmps=_tmps.Resize( _tmps.Length*2 )
			For Local i=_tmp Until _tmps.Length
				_tmps[i]=New Mat4
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
	
	Function InitTmps:Mat4[]()
		Local tmps:=New Mat4[256]
		For Local i=0 Until tmps.Length
			tmps[i]=New Mat4
		Next
		Return tmps
	End

End

Function FrustumMatrix:Mat4( left#,right#,bottom#,top#,near#,far# )
	Local near2:=near*2
	Local w:=right-left
	Local h:=top-bottom
	Local d:=far-near
	Local t:=Mat4.AllocTmp()
	
	t.ix=near2/w
	t.iy=0
	t.iz=0
	t.iw=0
	
	t.jx=0
	t.jy=near2/h
	t.jz=0
	t.jw=0
	
	t.kx=(right+left)/w
	t.ky=(top+bottom)/h
	t.kz=(far+near)/d
	t.kw=1
	
	t.tx=0
	t.ty=0
	t.tz=-(far*near2)/d
	t.tw=0
	
	Return t

#If TARGET="xna" Or TARGET="flash"
	'For molehill/d3d.
	t.kz=far/d
	t.tz=-t.kz*near
#Endif

End

Function TranslationMatrix:Mat4( tx#,ty#,tz# )
	Local t:=Mat4.Tmp()
	t.tx=tx
	t.ty=ty
	t.tz=tz
	Return t
End

Function TranslationMatrix:Mat4( t:Vec3 )
	Return TranslationMatrix( t.x,t.y,t.z )
End

Function YawMatrix:Mat4( yaw# )
	Local t:=Mat4.Tmp()
	Local s:=Sin(yaw),c:=Cos(yaw)
	t.ix= c ; t.iz=s
	t.kx=-s ; t.kz=c
	Return t
End Function

Function PitchMatrix:Mat4( pitch# )
	Local t:=Mat4.Tmp()
	Local s:=Sin(pitch),c:=Cos(pitch)
	t.jy= c ; t.jz=s
	t.ky=-s ; t.kz=c
	Return t
End Function

Function RollMatrix:Mat4( roll# )
	Local t:=Mat4.Tmp()
	Local s:=Sin(roll),c:=Cos(roll)
	t.ix= c ; t.iy=s
	t.jx=-s ; t.jy=c
	Return t
End Function
	
'pitch, yaw, roll matrix - actually computed in yaw, pitch, roll order for gamey use.
'
Function RotationMatrix:Mat4( pitch#,yaw#,roll# )
	Local t:Mat4
	If yaw
		t=YawMatrix(yaw)
		If pitch t=t.Times( PitchMatrix(pitch) )
		If roll t=t.Times( RollMatrix(roll) )
	Else If pitch
		t=PitchMatrix(pitch)
		If roll t=t.Times( RollMatrix(roll) )
	Else If roll
		t=RollMatrix(roll)
	Else
		t=Mat4.Tmp()
	Endif
	Return t
End

Function RotationMatrix:Mat4( r:Vec3 )
	Return RotationMatrix( r.x,r.y,r.z )
End

Function ScaleMatrix:Mat4( sx#,sy#,sz# )
	Local t:=Mat4.Tmp()
	t.ix=sx
	t.jy=sy
	t.kz=sz
	Return t
End

Function ScaleMatrix:Mat4( s:Vec3 )
	Return ScaleMatrix( s.x,s.y,s.z )
End

'trans/rot/scale matrix - in that order for gamey use
'
Function TRSMatrix:Mat4( tx#,ty#,tz#, rx#,ry#,rz#, sx#,sy#,sz# )
	Local t:Mat4
	If tx Or ty Or tz
		t=TranslationMatrix( tx,ty,tz )
		If rx Or ry Or rz t=t.Times( RotationMatrix( rx,ry,rz ) )
		If sx<>1 Or sy<>1 Or sz<>1 t=t.Times( ScaleMatrix( sx,sy,sz ) )
	Else If rx Or ry Or rz
		t=RotationMatrix( rx,ry,rz )
		If sx<>1 Or sy<>1 Or sz<>1 t=t.Times( ScaleMatrix( sx,sy,sz ) )
	Else If sx<>1 Or sy<>1 Or sz<>1
		t=ScaleMatrix( sx,sy,sz )
	Else
		t=Mat4.Tmp()
	Endif
	Return t
End

Function TRSMatrix:Mat4( t:Vec3,r:Vec3,s:Vec3 )
	Return TRSMatrix( t.x,t.y,t.z, r.x,r.y,r.z, s.x,s.y,s.z )
End
