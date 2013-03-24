
'#ANDROID_NATIVE_GL_ENABLED=True	'to run on android 2.2 - WARNING: uses native code.

Import opengl.gles20

Import mojo

Import geom

Global MVPMatrix:=New Mat4

Function CheckGL()
	Local err=glGetError()
	If err Error "GL Error "+err
End

Class Vertex
	Field x#,y#,z#
	Field nx#,ny#,nz#
	Field s#,t#
	
	Method New( x#,y#,z#,nx#=0,ny#=0,nz#=0,s#=0,t#=0 )
		Self.x=x;Self.y=y;Self.z=z
		Self.nx=nx;Self.ny=ny;Self.nz=nz
		Self.s=s;Self.t=t
	End
	
	Method Equals?( v:Vertex )
		Return x=v.x And y=v.y And z=v.z And nx=v.nx And ny=v.ny And nz=v.nz And s=v.s And t=v.t
	End
	
	Method Compare( with:Vertex )
		'memcmp!
		If x<with.x Return -1
		If x>with.x Return 1
		If y<with.y Return -1
		If y>with.y Return 1
		If z<with.z Return -1
		If z>with.z Return 1
		If nx<with.nx Return -1
		If nx>with.nx Return 1
		If ny<with.ny Return -1
		If ny>with.ny Return 1
		If nz<with.nz Return -1
		If nz>with.nz Return 1
		If s<with.s Return -1
		If s>with.s Return 1
		If t<with.t Return -1
		If t>with.t Return 1
		Return 0
	End
End

Class VertexMap<T> Extends Map<Vertex,T>

	Method Compare( lhs:Vertex,rhs:Vertex )
		Return lhs.Compare( rhs )
	End

End

Class VertexStack Extends Stack<Vertex>

	Method Equals?( lhs:Vertex,rhs:Vertex )
		Return lhs.Equals( rhs )
	End

	Method Compare( lhs:Vertex,rhs:Vertex )
		Return lhs.Compare( rhs )
	End

End

Class Shader

	Method New( vsource$,fsource$ )
		Local p[1]
		
		Local vshader:=glCreateShader( GL_VERTEX_SHADER )
		glShaderSource vshader,vsource
		glCompileShader vshader
		glGetShaderiv vshader,GL_COMPILE_STATUS,p
		If Not p[0] Print "Failed to compile vshader:"+glGetShaderInfoLog( vshader )

		Local fshader:=glCreateShader( GL_FRAGMENT_SHADER )
		glShaderSource fshader,fsource
		glCompileShader fshader
		glGetShaderiv fshader,GL_COMPILE_STATUS,p
		If Not p[0] Print "Failed to compile fshader:"+glGetShaderInfoLog( fshader )
		
		program=glCreateProgram()
		glAttachShader program,vshader
		glAttachShader program,fshader
		glLinkProgram program
		glGetProgramiv program,GL_LINK_STATUS,p
		If Not p[0] Print "Failed to link program:"+glGetProgramInfoLog( program )
	End
	
	Method Bind()
		glUseProgram program
		CheckGL
	End
	
	Private
	
	Field program
	
End

Class Material

	Method New( shader:Shader )
		Self.shader=shader
		color_loc=glGetUniformLocation( shader.program,"Color" )
		texture_loc=glGetUniformLocation( shader.program,"Texture" )
	End
	
	Method SetColor( r#,g#,b#,a# )
		Self.r=r;Self.g=g;Self.b=b;Self.a=a
	End
	
	Method SetTexture( tex )
		Self.tex=tex
	End
	
	Method Bind()
		glActiveTexture GL_TEXTURE0
		glBindTexture GL_TEXTURE_2D,tex
		shader.Bind
		glUniform4f color_loc,r,g,b,a
		glUniform1i texture_loc,0
		CheckGL
	End
	
	Private
	
	Field shader:Shader
	Field r#,g#,b#,a#,tex
	Field color_loc,texture_loc
	
End

Class Mesh

	Method New( material:Material )
		Self.material=material
		pos_loc=glGetAttribLocation( material.shader.program,"Position" )
		norm_loc=glGetAttribLocation( material.shader.program,"Normal" )
		texc_loc=glGetAttribLocation( material.shader.program,"TexCoords" )
		mvp_loc=glGetUniformLocation( material.shader.program,"MVPMatrix" )
	End

	Method AddVertex( v:Vertex )
		Local i
		If vmap.Contains( v )
			i=vmap.Get( v )
		Else
			i=vs.Length
			vmap.Set v,i
			vs.Push v
		Endif
		is.Push i
	End

	Method AddVertex( x#,y#,z#,nx#,ny#,nz#,s#,t# )
		AddVertex New Vertex( x,y,z,nx,ny,nz,s,t )
	End
	
	Method Bind()
	
		material.Bind
	
		If vbo
			glBindBuffer GL_ARRAY_BUFFER,vbo
			glBindBuffer GL_ELEMENT_ARRAY_BUFFER,ibo
		Else
			vcount=vs.Length
			icount=is.Length
			
			Local vdata:=New DataBuffer( vcount*32 )
			For Local i:=0 Until vcount
				Local p=i*32
				Local v:=vs.Get( i )
				vdata.PokeFloat p,v.x
				vdata.PokeFloat p+4,v.y
				vdata.PokeFloat p+8,v.z
				vdata.PokeFloat p+12,v.nx
				vdata.PokeFloat p+16,v.ny
				vdata.PokeFloat p+20,v.nz
				vdata.PokeFloat p+24,v.s
				vdata.PokeFloat p+28,v.t
			Next
			
			Local idata:=New DataBuffer( icount*2 )
			For Local i:=0 Until icount
				idata.PokeShort i*2,is.Get(i)
			Next
			
			vbo=glCreateBuffer()
			glBindBuffer GL_ARRAY_BUFFER,vbo
			glBufferData GL_ARRAY_BUFFER,vdata.Length,vdata,GL_STATIC_DRAW
			
			ibo=glCreateBuffer()
			glBindBuffer GL_ELEMENT_ARRAY_BUFFER,ibo
			glBufferData GL_ELEMENT_ARRAY_BUFFER,idata.Length,idata,GL_STATIC_DRAW
		
		Endif
		
		glEnableVertexAttribArray pos_loc
		glVertexAttribPointer pos_loc,3,GL_FLOAT,False,32,0

		If norm_loc<>-1
			glEnableVertexAttribArray norm_loc
			glVertexAttribPointer norm_loc,3,GL_FLOAT,False,32,12
		Endif

		If texc_loc<>-1
			glEnableVertexAttribArray texc_loc
			glVertexAttribPointer texc_loc,2,GL_FLOAT,False,32,24
		Endif
		
		CheckGL
	End
	
	Method Render()
		'transpose MUST be false - took me an hour or two to find that out!		
		glUniformMatrix4fv mvp_loc,1,False,MVPMatrix.ToArray()
		
		glDrawElements GL_TRIANGLES,icount,GL_UNSIGNED_SHORT,0
		
		CheckGL
	End
	
	Private
	
	Field material:Material

	Field vbo,ibo,vcount,icount
	Field pos_loc,norm_loc,texc_loc
	Field mvp_loc
	
	Field vs:=New VertexStack
	Field is:=New IntStack
	Field vmap:=New VertexMap<Int>

End

Class ObjLoader

	Method LoadMesh:Mesh( path$,material:Material )
	
		Local mesh:=New Mesh( material )
	
		Local v:=New FloatStack
		Local vt:=New FloatStack
		Local vn:=New FloatStack
		
		Print "Load obj file:"+path
		
		For Local line:=Eachin LoadString( path ).Split( "~n" )
		
			line=line.Trim()
			If Not line Or line.StartsWith( "#" ) Continue
			
			Print line
			
			Local bits:=line.Split( " " )
			Select bits[0]
			Case "v"
				If bits.Length=4
					v.Push Float( bits[1] )
					v.Push Float( bits[2] )
					v.Push Float( bits[3] )
				Else
					Error "obj error"
				Endif
			Case "vn"
				If bits.Length=4
					vn.Push Float( bits[1] )
					vn.Push Float( bits[2] )
					vn.Push Float( bits[3] )
				Else
					Error "obj error"
				Endif
			Case "vt"
				If bits.Length=3
					vt.Push Float( bits[1] )
					vt.Push Float( bits[2] )
				Else
					Error "obj error"
				Endif
			Case "f"
				If bits.Length=4
					For Local i=1 To 3
						Local bits2:=bits[i].Split( "/" )
						If bits2.Length=3
							Local vi:=(Int(bits2[0])-1)*3
							Local vti:=(Int(bits2[1])-1)*2
							Local vni:=(Int(bits2[2])-1)*3
							mesh.AddVertex v.Get(vi),v.Get(vi+1),v.Get(vi+2),vn.Get(vni),vn.Get(vni+1),vn.Get(vni+2),vt.Get(vti),vt.Get(vti+1)
						Else
							Error "obj error"
						Endif
					Next
				Else
					Error "obj error"
				Endif
			Case "mtllib"
			Case "usemtl"
			Case "s"
			Default
				Error "obj error"
			End
		Next
		
		Return mesh
	End

End

Class MyApp Extends App

	Field rot#
	
	Field obj_z:=30.0
	
	Field mesh:Mesh

	Method OnCreate()
	
		Local tex:=glCreateTexture()
		
		glBindTexture GL_TEXTURE_2D,tex
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE
		glTexImage2D GL_TEXTURE_2D,0,GL_RGBA,GL_RGBA,GL_UNSIGNED_BYTE,"monkey://data/mandril.jpg"
		glGenerateMipmap GL_TEXTURE_2D
		CheckGL
			
		Local vsource:=LoadString( "vshader.txt" )
		Local fsource:=LoadString( "fshader.txt" )
		Local shader:=New Shader( vsource,fsource )

		Local material:=New Material( shader )
		material.SetColor 1,1,0,1
		material.SetTexture tex
	
		Local loader:=New ObjLoader
		
		mesh=loader.LoadMesh( "cube.txt",material )
		
		SetUpdateRate 60

	End
	
	Method OnUpdate()
	
		ClearTmps

		If MouseDown()
			If MouseY()<DeviceHeight()/2
				obj_z-=.1
			Else
				obj_z+=.1
			Endif
		Endif
	
		rot+=1
	End
	
	Method OnRender()
	
		ClearTmps
	
		glViewport 0,0,DeviceWidth,DeviceHeight
		
		glClearColor 1,0,.5,1
		glClear GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT
		
		glEnable GL_DEPTH_TEST
		glDepthFunc GL_LESS
		
		mesh.Bind

		Local projMatrix:=New Mat4
		projMatrix.Set FrustumMatrix( -1,1,-1,1,1,100 )
			
		For Local x#=-25 To 25 Step 5
		
			For Local y=-25 To 25 Step 5
			
				PushTmps
			
				Local modelMatrix:=TRSMatrix( x,y,obj_z, 0,rot,0, 1,1,1 )
				
				MVPMatrix.Set projMatrix.Times( modelMatrix )
		
				mesh.Render
				
				PopTmps

			Next
		Next

	End
	
	Method OnSuspend()
		Error ""	'budget way to handle loss of context!
	End
	
End

Function Main()

	New MyApp
	
End
