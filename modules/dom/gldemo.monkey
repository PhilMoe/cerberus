
Import dom

Class MyApp Extends EventListener

	Field canvas:HTMLCanvasElement
	
	Field gl:WebGLRenderingContext
	
	Method CheckGL()
		Local err:=gl.getError()
		If err=gl.NO_ERROR Return
		Print "ERROR! gl.getError="+err
	End
	
	Method New()
	
		Print "Tiny OpenGL App!"
	
		canvas=HTMLCanvasElement( document.getElementById( "GameCanvas" ) )
		
		gl=WebGLRenderingContext( canvas.getContext( "experimental-webgl" ) )

		'Create/compile vertex shader...
		'
		Local vsrc:=""+
		"attribute vec2 vertexPosition;"+
		"void main(){"+
		"   gl_Position=vec4( vertexPosition,0.0,1.0 );"+
		"}"
		Local vshader:=gl.createShader( gl.VERTEX_SHADER )
		gl.shaderSource vshader,vsrc
		gl.compileShader vshader
		If Not gl.getShaderParameter( vshader,gl.COMPILE_STATUS )
			Print "Error compiling vshader:"+gl.getShaderInfoLog( vshader )
		Endif
		CheckGL

		'Create/compile fragment shader...
		'
		Local fsrc:=""+
		"void main(){"+
		"   gl_FragColor=vec4( 0.0,1.0,0.0,1.0 );"+
		"}"
		Local fshader:=gl.createShader( gl.FRAGMENT_SHADER )
		gl.shaderSource fshader,fsrc
		gl.compileShader fshader
		If Not gl.getShaderParameter( fshader,gl.COMPILE_STATUS )
			Print "Error compiling vshader:"+gl.getShaderInfoLog( fshader )
		Endif
		CheckGL

		'Create/link/use shader program
		'
		Local program:=gl.createProgram()
		gl.attachShader program,vshader
		gl.attachShader program,fshader
		gl.linkProgram program
		If Not gl.getProgramParameter( program,gl.LINK_STATUS )
			Print "Error linking shader program"
		Endif
		gl.useProgram program
		Local vp:=gl.getAttribLocation( program,"vertexPosition" )
		CheckGL

		'Create/bind vertex buffer
		'
		Local arrbuf:=createArrayBuffer( 24 )
		Local verts:=createFloat32Array( arrbuf,0,6 )
		verts[0]=0
		verts[1]=1
		verts[2]=1
		verts[3]=-1
		verts[4]=-1
		verts[5]=-1
		Local vertbuf:=gl.createBuffer()
		gl.bindBuffer gl.ARRAY_BUFFER,vertbuf
		gl.bufferData gl.ARRAY_BUFFER,arrbuf,gl.STATIC_DRAW
		gl.enableVertexAttribArray vp
		gl.vertexAttribPointer vp,2,gl.FLOAT_,False,0,0
		CheckGL
		
		'Draw!
		'
		gl.clearColor 1,1,0,1
		gl.clear gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT
		gl.drawArrays gl.TRIANGLES,0,3
		CheckGL
		
	End
	
End

Function Main()

	New MyApp
	
End
