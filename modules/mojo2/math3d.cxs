
Global Mat4Identity:=[1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0]

Function Vec4Init:Void( x:Float,y:Float,z:Float,w:Float,r:Float[] )
	r[0]=x;r[1]=y;r[2]=z;r[3]=w
End

Function Vec4Copy:Void( v:Float[],r:Float[] )
	r[0]=v[0];r[1]=v[1];r[2]=v[2];r[3]=v[3]
End

Function Vec4Copy:Void( v:Float[],r:Float[],src:Int,dst:Int )
	r[0+dst]=v[0+src];r[1+dst]=v[1+src];r[2+dst]=v[2+src];r[3+dst]=v[3+src]
End

Function Mat4New:Float[]()
	Return [1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0]
End

Function Mat4Init:Void( ix:Float,jy:Float,kz:Float,tw:Float,r:Float[] )
	r[0]= ix; r[1]=  0; r[2]=  0; r[3]=  0
	r[4]=  0; r[5]= jy; r[6]=  0; r[7]=  0
	r[8]=  0; r[9]=  0; r[10]=kz; r[11]= 0
	r[12]= 0; r[13]= 0; r[14]= 0; r[15]=tw
End

Function Mat4Init:Void( ix:Float,iy:Float,iz:Float,iw:Float,jx:Float,jy:Float,jz:Float,jw:Float,kx:Float,ky:Float,kz:Float,kw:Float,tx:Float,ty:Float,tz:Float,tw:Float,r:Float[] )
	r[0]= ix;r[1]= iy;r[2]= iz;r[3]= iw
	r[4]= jx;r[5]= jy;r[6]= jz;r[7]= jw
	r[8]= kx;r[9]= ky;r[10]=kz;r[11]=kw
	r[12]=tx;r[13]=ty;r[14]=tz;r[15]=tw
End

Function Mat4Init:Void( r:Float[] )
	Mat4Init 1,1,1,1,r
End

Function Mat4Copy:Void( m:Float[],r:Float[] )
	r[0]=m[0];r[1]=m[1];r[2]=m[2];r[3]=m[3]
	r[4]=m[4];r[5]=m[5];r[6]=m[6];r[7]=m[7]
	r[8]=m[8];r[9]=m[9];r[10]=m[10];r[11]=m[11]
	r[12]=m[12];r[13]=m[13];r[14]=m[14];r[15]=m[15]
End

Function Mat4Ortho:Void( left:Float,right:Float,bottom:Float,top:Float,znear:Float,zfar:Float,r:Float[] )
	Local w:=right-left,h:=top-bottom,d:=zfar-znear
	Mat4Init 2.0/w,0,0,0, 0,2.0/h,0,0, 0,0,2.0/d,0, -(right+left)/w,-(top+bottom)/h,-(zfar+znear)/d,1,r
End

Function Mat4Frustum:Void( left:Float,right:Float,bottom:Float,top:Float,znear:Float,zfar:Float,r:Float[] )	
	Local w:=right-left,h:=top-bottom,d:=zfar-znear,znear2:=znear*2
	Mat4Init znear2/w,0,0,0, 0,znear2/h,0,0, (right+left)/w,(top+bottom)/h,(zfar+znear)/d,1, 0,0,-(zfar*znear2)/d,0, r
End

Function Mat4Transpose:Void( m:Float[],r:Float[] )
	Mat4Init m[0],m[4],m[8],m[12], m[1],m[5],m[9],m[13], m[2],m[6],m[10],m[14], m[3],m[7],m[11],m[15],r
End

Function Mat4Inverse:Void( m:Float[],r:Float[] )
	r[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10]
	r[4] =-m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10]
	r[8] = m[4] * m[9]  * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9]
	r[12]=-m[4] * m[9]  * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9]
	r[1] =-m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10]
	r[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10]
	r[9] =-m[0] * m[9]  * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9]
	r[13]= m[0] * m[9]  * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9]
	r[2] = m[1] * m[6]  * m[15] - m[1] * m[7]  * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7]  - m[13] * m[3] * m[6]
	r[6] =-m[0] * m[6]  * m[15] + m[0] * m[7]  * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7]  + m[12] * m[3] * m[6]
	r[10]= m[0] * m[5]  * m[15] - m[0] * m[7]  * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7]  - m[12] * m[3] * m[5]
	r[14]=-m[0] * m[5]  * m[14] + m[0] * m[6]  * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6]  + m[12] * m[2] * m[5]
	r[3] =-m[1] * m[6]  * m[11] + m[1] * m[7]  * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9]  * m[2] * m[7]  + m[9]  * m[3] * m[6]
	r[7] = m[0] * m[6]  * m[11] - m[0] * m[7]  * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8]  * m[2] * m[7]  - m[8]  * m[3] * m[6]
	r[11]=-m[0] * m[5]  * m[11] + m[0] * m[7]  * m[9]  + m[4] * m[1] * m[11] - m[4] * m[3] * m[9]  - m[8]  * m[1] * m[7]  + m[8]  * m[3] * m[5]
	r[15]= m[0] * m[5]  * m[10] - m[0] * m[6]  * m[9]  - m[4] * m[1] * m[10] + m[4] * m[2] * m[9]  + m[8]  * m[1] * m[6]  - m[8]  * m[2] * m[5]
	Local c:=1.0 / (m[0] * r[0] + m[1] * r[4] + m[2] * r[8] + m[3] * r[12])
	For Local i:=0 Until 16
		r[i]*=c
	Next
End

Function Mat4Multiply:Void( m:Float[],n:Float[],r:Float[] )
	Mat4Init(
	m[0]*n[0]  + m[4]*n[1]  + m[8]*n[2]  + m[12]*n[3],  m[1]*n[0]  + m[5]*n[1]  + m[9]*n[2]  + m[13]*n[3],  m[2]*n[0]  + m[6]*n[1]  + m[10]*n[2]  + m[14]*n[3],  m[3]*n[0]  + m[7]*n[1]  + m[11]*n[2]  + m[15]*n[3],
	m[0]*n[4]  + m[4]*n[5]  + m[8]*n[6]  + m[12]*n[7],  m[1]*n[4]  + m[5]*n[5]  + m[9]*n[6]  + m[13]*n[7],  m[2]*n[4]  + m[6]*n[5]  + m[10]*n[6]  + m[14]*n[7],  m[3]*n[4]  + m[7]*n[5]  + m[11]*n[6]  + m[15]*n[7],
	m[0]*n[8]  + m[4]*n[9]  + m[8]*n[10] + m[12]*n[11], m[1]*n[8]  + m[5]*n[9]  + m[9]*n[10] + m[13]*n[11], m[2]*n[8]  + m[6]*n[9]  + m[10]*n[10] + m[14]*n[11], m[3]*n[8]  + m[7]*n[9]  + m[11]*n[10] + m[15]*n[11],
	m[0]*n[12] + m[4]*n[13] + m[8]*n[14] + m[12]*n[15], m[1]*n[12] + m[5]*n[13] + m[9]*n[14] + m[13]*n[15], m[2]*n[12] + m[6]*n[13] + m[10]*n[14] + m[14]*n[15], m[3]*n[12] + m[7]*n[13] + m[11]*n[14] + m[15]*n[15],r )
End

Function Mat4Transform:Void( m:Float[],v:Float[],r:Float[] )
	Vec4Init( 
	m[0]*v[0] + m[4]*v[1] + m[8] *v[2] + m[12]*v[3],
	m[1]*v[0] + m[5]*v[1] + m[9] *v[2] + m[13]*v[3],
	m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]*v[3],
	m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]*v[3],r )
End

Function Mat4Project:Void( m:Float[],v:Float[],r:Float[] )
	Vec4Init( 
	m[0]*v[0] + m[4]*v[1] + m[8] *v[2] + m[12]*v[3],
	m[1]*v[0] + m[5]*v[1] + m[9] *v[2] + m[13]*v[3],
	m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]*v[3],
	m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]*v[3],r )
	r[0]/=r[3];r[1]/=r[3];r[2]/=r[3];r[3]=1
End

Function Mat4Roll:Void( rz:Float,m:Float[] )
End

Function Mat4Scale:Void( x:Float,y:Float,z:Float,m:Float[] )
	m[0]=x;m[1]=0;m[2]=0;m[3]=0
	m[4]=0;m[5]=y;m[6]=0;m[7]=0
	m[8]=0;m[9]=0;m[10]=1;m[11]=0
	m[12]=0;m[13]=0;m[14]=0;m[15]=1
End
