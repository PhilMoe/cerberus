
Import opengl.gles20

Private

Global tmpi:Int[16]

Public

Function glCheck:Void()
	Local err:=glGetError()
	If err=GL_NO_ERROR Return
	Error "GL ERROR! err="+err
End

Function glPushTexture2d:Void( tex:Int )
	glGetIntegerv GL_TEXTURE_BINDING_2D,tmpi
	glBindTexture GL_TEXTURE_2D,tex
End

Function glPopTexture2d:Void()
	glBindTexture GL_TEXTURE_2D,tmpi[0]
End

Function glPushFramebuffer:Void( framebuf:Int )
	glGetIntegerv GL_FRAMEBUFFER_BINDING,tmpi
	glBindFramebuffer GL_FRAMEBUFFER,framebuf
End

Function glPopFramebuffer:Void()
	glBindFramebuffer GL_FRAMEBUFFER,tmpi[0]
End

Function glCompile:Int( type:Int,source:String )
	
	#If TARGET<>"glfw" Or GLFW_USE_ANGLE_GLES20
		source="precision mediump float;~n"+source
	#Endif
	
	Local shader:=glCreateShader( type )
	glShaderSource shader,source
	glCompileShader shader
	glGetShaderiv shader,GL_COMPILE_STATUS,tmpi
	If Not tmpi[0] 
		Print "Failed to compile fragment shader:"+glGetShaderInfoLog( shader )
		Local lines:=source.Split( "~n" )
		For Local i:=0 Until lines.Length
			Print (i+1)+":~t"+lines[i]
		Next
		Error "Compile fragment shader failed"
	Endif
	Return shader

End

Function glLink:Void( program:Int )
	glLinkProgram program
	glGetProgramiv program,GL_LINK_STATUS,tmpi
	If Not tmpi[0] Error "Failed to link program:"+glGetProgramInfoLog( program )
End




