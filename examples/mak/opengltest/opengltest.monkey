
#OPENGL_DEPTH_BUFFER_ENABLED=True

Import opengl.gles11

Import mojo

Class GLApp Extends App

	Field tex		'texture
	Field vbo,ibo	'vertex buffer,index buffer
	Field rot#

	Method OnCreate()
	
		SetUpdateRate 60

	End
	
	Field _inited
	
	Method Init()

		If _inited Return
		_inited=True
		
		'create texture object
		Local texs[1]
		glGenTextures 1,texs
		tex=texs[0]
		
		'Load/bind texture data
		Local info[2]
		Local tbuf:=LoadImageData( "monkey://data/Grass_1.png",info )
		Print "width="+info[0]
		Print "height="+info[1]
		If Not tbuf Error ""
		glBindTexture GL_TEXTURE_2D,tex
		glPixelStorei GL_UNPACK_ALIGNMENT,1
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE
		glTexParameteri GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE
		glTexImage2D GL_TEXTURE_2D,0,GL_RGBA,256,256,0,GL_RGBA,GL_UNSIGNED_BYTE,tbuf

		'create buffer objects
		Local bufs[2]
		glGenBuffers 2,bufs
		vbo=bufs[0]
		ibo=bufs[1]

		'create VBO
		Local vtxs:=[-1.0,+1.0,0.0,0.0, +1.0,+1.0,1.0,0.0, +1.0,-1.0,1.0,1.0, -1.0,-1.0,0.0,1.0]
		'
		Local vbuf:=New DataBuffer( vtxs.Length*4 )
		For Local i=0 Until vtxs.Length
			vbuf.PokeFloat i*4,vtxs[i]
		Next
		glBindBuffer GL_ARRAY_BUFFER,vbo
		glBufferData GL_ARRAY_BUFFER,vbuf.Length,vbuf,GL_STATIC_DRAW

		'create IBO		
		Local idxs:=[0,1,2,0,2,3]
		'
		Local ibuf:=New DataBuffer( idxs.Length*2 )
		For Local i=0 Until idxs.Length
			ibuf.PokeShort i*2,idxs[i]
		Next
		glBindBuffer GL_ELEMENT_ARRAY_BUFFER,ibo
		glBufferData GL_ELEMENT_ARRAY_BUFFER,ibuf.Length,ibuf,GL_STATIC_DRAW
		
	End
	
	Method OnSuspend()
		Error ""
	End
	
	Method OnRender()

		Init
		
		glActiveTexture GL_TEXTURE0
		glClientActiveTexture GL_TEXTURE0
		
		glViewport 0,0,DeviceWidth,DeviceHeight
	
		glDisable( GL_BLEND );
		
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity
		glFrustumf -1,1,-1,1,1,100
		
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity
		
		rot+=1
		glRotatef rot,0,0,1

		glClearColor 0,0,.5,1
		glClear GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT
		
		glEnableClientState GL_VERTEX_ARRAY
		glEnableClientState GL_TEXTURE_COORD_ARRAY
		glDisableClientState GL_COLOR_ARRAY

		glBindBuffer GL_ARRAY_BUFFER,vbo
		glVertexPointer 2,GL_FLOAT,16,0
		glTexCoordPointer 2,GL_FLOAT,16,8
		
		glBindBuffer GL_ELEMENT_ARRAY_BUFFER,ibo

		glEnable GL_TEXTURE_2D
		glBindTexture GL_TEXTURE_2D,tex

		glEnable GL_DEPTH_TEST
		glDepthFunc GL_LESS		
		glDepthMask True
		
		glLoadIdentity
		glTranslatef 0,-1,-2.5
		glRotatef 90,1,0,0
		glDrawElements GL_TRIANGLES,6,GL_UNSIGNED_SHORT,0
		
		glLoadIdentity
		glTranslatef 0,0,-2.5
		glRotatef rot,0,0,1
		glDrawElements GL_TRIANGLES,6,GL_UNSIGNED_SHORT,0
		
	End
	
End

Function Main()
	New GLApp
End
