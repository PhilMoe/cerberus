
#if _WIN32

#define INIT_GL_EXTS 1

//1.3
void (__stdcall* glActiveTexture)( GLenum texture );
void (__stdcall* glClientActiveTexture)( GLenum texture );
void (__stdcall* glCompressedTexImage2D)( GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLint border,GLsizei imageSize,const GLvoid *data );
void (__stdcall* glCompressedTexSubImage2D)( GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLsizei imageSize,const GLvoid *data );
void (__stdcall* glMultiTexCoord4f)( GLenum target,GLfloat s,GLfloat t,GLfloat r,GLfloat q );
void (__stdcall* glSampleCoverage)( GLclampf value,GLboolean invert );

//1.4
void (__stdcall* glPointParameterf)( GLenum pname,GLfloat param );

//1.5
void (__stdcall* glGenBuffers)( GLsizei n,GLuint *buffers );
void (__stdcall* glDeleteBuffers)( GLsizei n,GLuint *buffers );
void (__stdcall* glBufferData)( GLenum target,GLsizei size,const GLvoid *data,GLenum usage );
void (__stdcall* glBufferSubData)( GLenum target,GLsizei offset,GLsizei size,const GLvoid *data );
void (__stdcall* glBindBuffer)( GLenum target,GLuint buffer );
int  (__stdcall* glIsBuffer)( GLuint buffer );
void (__stdcall* glGetBufferParameteriv)( GLenum target,GLenum pname,GLint *params );

void Init_GL_Exts(){
	
	const char *p=(const char*)glGetString( GL_VERSION );
	int v=(p[0]-'0')*10+(p[2]-'0');
	
	if( v>=13 ){
		(void*&)glActiveTexture=(void*)wglGetProcAddress( "glActiveTexture" );
		(void*&)glClientActiveTexture=(void*)wglGetProcAddress( "glClientActiveTexture" );
		(void*&)glCompressedTexImage2D=(void*)wglGetProcAddress( "glCompressedTexImage2D" );
		(void*&)glCompressedTexSubImage2D=(void*)wglGetProcAddress( "glCompressedTexSubImage2D" );
		(void*&)glMultiTexCoord4f=(void*)wglGetProcAddress( "glMultiTexCoord4f" );
		(void*&)glSampleCoverage=(void*)wglGetProcAddress( "glSampleCoverage" );
	}else{
		(void*&)glActiveTexture=(void*)wglGetProcAddress( "glActiveTextureARB" );
		(void*&)glClientActiveTexture=(void*)wglGetProcAddress( "glClientActiveTextureARB" );
		(void*&)glCompressedTexImage2D=(void*)wglGetProcAddress( "glCompressedTexImage2DARB" );
		(void*&)glCompressedTexSubImage2D=(void*)wglGetProcAddress( "glCompressedTexSubImage2DARB" );
		(void*&)glMultiTexCoord4f=(void*)wglGetProcAddress( "glMultiTexCoord4fARB" );
		(void*&)glSampleCoverage=(void*)wglGetProcAddress( "glSampleCoverageARB" );
	}
	if( v>=14 ){
		(void*&)glPointParameterf=(void*)wglGetProcAddress( "glPointParameterf" );
	}else{
		(void*&)glPointParameterf=(void*)wglGetProcAddress( "glPointParameterfARB" );
	}
	if( v>=15 ){
		(void*&)glGenBuffers=(void*)wglGetProcAddress( "glGenBuffers" );
		(void*&)glDeleteBuffers=(void*)wglGetProcAddress( "glDeleteBuffers" );
		(void*&)glBufferData=(void*)wglGetProcAddress( "glBufferData" );
		(void*&)glBufferSubData=(void*)wglGetProcAddress( "glBufferSubData" );
		(void*&)glBindBuffer=(void*)wglGetProcAddress( "glBindBuffer" );
		(void*&)glIsBuffer=(void*)wglGetProcAddress( "glIsBuffer" );
		(void*&)glGetBufferParameteriv=(void*)wglGetProcAddress( "glGetBufferParameteriv" );
	}else{
		(void*&)glGenBuffers=(void*)wglGetProcAddress( "glGenBuffersARB" );
		(void*&)glDeleteBuffers=(void*)wglGetProcAddress( "glDeleteBuffersARB" );
		(void*&)glBufferData=(void*)wglGetProcAddress( "glBufferDataARB" );
		(void*&)glBufferSubData=(void*)wglGetProcAddress( "glBufferSubDataARB" );
		(void*&)glBindBuffer=(void*)wglGetProcAddress( "glBindBufferARB" );
		(void*&)glIsBuffer=(void*)wglGetProcAddress( "glIsBufferARB" );
		(void*&)glGetBufferParameteriv=(void*)wglGetProcAddress( "glGetBufferParameterivARB" );
	}
}

#endif

BBDataBuffer *BBLoadImageData( BBDataBuffer *buf,String path,Array<int> info ){
	int width,height,depth;
	unsigned char *data=BBGlfwGame::GlfwGame()->LoadImageData( path,&width,&height,&depth );
	if( !data || depth<1 || depth>4 ) return 0;
	
	int size=width*height;
	
	if( !buf->_New( size*4 ) ) return 0;
	
	unsigned char *src=data,*dst=(unsigned char*)buf->WritePointer();
	int i;
	
	switch( depth ){
	case 1:for( i=0;i<size;++i ){ *dst++=*src;*dst++=*src;*dst++=*src++;*dst++=255; } break;
	case 2:for( i=0;i<size;++i ){ *dst++=*src;*dst++=*src;*dst++=*src++;*dst++=*src++; } break;
	case 3:for( i=0;i<size;++i ){ *dst++=*src++;*dst++=*src++;*dst++=*src++;*dst++=255; } break;
	case 4:for( i=0;i<size;++i ){ *dst++=*src++;*dst++=*src++;*dst++=*src++;*dst++=*src++; } break;
	}
	
	if( info.Length()>0 ) info[0]=width;
	if( info.Length()>1 ) info[1]=height;
	
	free( data );
	
	return buf;
}

void _glGenBuffers( int n,Array<int> buffers,int offset ){
	glGenBuffers( n,(GLuint*)&buffers[offset] );
}

void _glDeleteBuffers( int n,Array<int> buffers,int offset ){
	glDeleteBuffers( n,(GLuint*)&buffers[offset] );
}

void _glGenTextures( int n,Array<int> textures,int offset ){
	glGenTextures( n,(GLuint*)&textures[offset] );
}

void _glDeleteTextures( int n,Array<int> textures,int offset ){
	glDeleteTextures( n,(GLuint*)&textures[offset] );
}

void _glClipPlanef( int plane,Array<Float> equation,int offset ){
	double buf[4];
	for( int i=0;i<4;++i ) buf[i]=equation[offset+i];
	glClipPlane( plane,buf );
}

void _glGetClipPlanef( int plane,Array<Float> equation,int offset ){
	double buf[4];
	glGetClipPlane( plane,buf );
	for( int i=0;i<4;++i ) equation[offset+i]=buf[i];
}

void _glFogfv( int pname,Array<Float> params,int offset ){
	glFogfv( pname,&params[offset] );
}

void _glGetFloatv( int pname,Array<Float> params,int offset ){
	glGetFloatv( pname,&params[offset] );
}

void _glGetLightfv( int target,int pname,Array<Float> params,int offset ){
	glGetLightfv( target,pname,&params[offset] );
}

void _glGetMaterialfv( int target,int pname,Array<Float> params,int offset ){
	glGetMaterialfv( target,pname,&params[offset] );
}

void _glGetTexEnvfv( int target,int pname,Array<Float> params,int offset ){
	glGetTexEnvfv( target,pname,&params[offset] );
}

void _glGetTexParameterfv( int target,int pname,Array<Float> params,int offset ){
	glGetTexParameterfv( target,pname,&params[offset] );
}

void _glLightfv( int target,int pname,Array<Float> params,int offset ){
	glLightfv( target,pname,&params[offset] );
}

void _glLightModelfv( int pname,Array<Float> params,int offset ){
	glLightModelfv( pname,&params[offset] );
}

void _glLoadMatrixf( Array<Float> params,int offset ){
	glLoadMatrixf( &params[offset] );
}

void _glMaterialfv( int target,int pname,Array<Float> params,int offset ){
	glMaterialfv( target,pname,&params[offset] );
}

void _glMultMatrixf( Array<Float> params,int offset ){
	glMultMatrixf( &params[offset] );
}

void _glTexEnvfv( int target,int pname,Array<Float> params,int offset ){
	glTexEnvfv( target,pname,&params[offset] );
}

void _glTexParameterfv( int target,int pname,Array<Float> params,int offset ){
	glTexParameterfv( target,pname,&params[offset] );
}

void _glGetIntegerv( int pname,Array<int> params,int offset ){
	glGetIntegerv( pname,&params[offset] );
}

String _glGetString( int name ){
	return String( glGetString( name ) );
}

void _glGetTexEnviv( int target,int pname,Array<int> params,int offset ){
	glGetTexEnviv( target,pname,&params[offset] );
}

void _glGetTexParameteriv( int target,int pname,Array<int> params,int offset ){
	glGetTexParameteriv( target,pname,&params[offset] );
}

void _glTexEnviv( int target,int pname,Array<int> params,int offset ){
	glTexEnviv( target,pname,&params[offset] );
}

void _glTexParameteriv( int target,int pname,Array<int> params,int offset ){
	glTexParameteriv( target,pname,&params[offset] );
}

void _glVertexPointer( int size,int type,int stride,BBDataBuffer *pointer ){
	glVertexPointer( size,type,stride,pointer->ReadPointer() );
}

void _glVertexPointer( int size,int type,int stride,int offset ){
	glVertexPointer( size,type,stride,(const void*)offset );
}

void _glColorPointer( int size,int type,int stride,BBDataBuffer *pointer ){
	glColorPointer( size,type,stride,pointer->ReadPointer() );
}

void _glColorPointer( int size,int type,int stride,int offset ){
	glColorPointer( size,type,stride,(const void*)offset );
}

void _glNormalPointer( int type,int stride,BBDataBuffer *pointer ){
	glNormalPointer( type,stride,pointer->ReadPointer() );
}

void _glNormalPointer( int type,int stride,int offset ){
	glNormalPointer( type,stride,(const void*)offset );
}

void _glTexCoordPointer( int size,int type,int stride,BBDataBuffer *pointer ){
	glTexCoordPointer( size,type,stride,pointer->ReadPointer() );
}

void _glTexCoordPointer( int size,int type,int stride,int offset ){
	glTexCoordPointer( size,type,stride,(const void*)offset );
}

void _glDrawElements( int mode,int count,int type,BBDataBuffer *indices ){
	glDrawElements( mode,count,type,indices->ReadPointer() );
}

void _glDrawElements( int mode,int count,int type,int offset ){
	glDrawElements( mode,count,type,(const GLvoid*)offset );
}

void _glGetBufferParameteriv( int target,int pname,Array<int> params,int offset ){
	glGetBufferParameteriv( target,pname,&params[offset] );
}

void _glBufferData( int target,int size,BBDataBuffer *data,int usage ){
	glBufferData( target,size,data->ReadPointer(),usage );
}

void _glBufferSubData( int target,int offset,int size,BBDataBuffer *data ){
	glBufferSubData( target,offset,size,data->ReadPointer() );
}

void _glTexImage2D( int target,int level,int internalformat,int width,int height,int border,int format,int type,BBDataBuffer *pixels ){
	glTexImage2D( target,level,internalformat,width,height,border,format,type,pixels->ReadPointer() );
}

void _glTexSubImage2D( int target,int level,int xoffset,int yoffset,int width,int height,int format,int type,BBDataBuffer *pixels ){
	glTexSubImage2D( target,level,xoffset,yoffset,width,height,format,type,pixels->ReadPointer() );
}

void _glCompressedTexImage2D( int target,int level,int internalformat,int width,int height,int border,int imageSize,BBDataBuffer *pixels ){
	glCompressedTexImage2D( target,level,internalformat,width,height,border,imageSize,pixels->ReadPointer() );
}

void _glCompressedTexSubImage2D( int target,int level,int xoffset,int yoffset,int width,int height,int format,int imageSize,BBDataBuffer *pixels ){
	glCompressedTexSubImage2D( target,level,xoffset,yoffset,width,height,format,imageSize,pixels->ReadPointer() );
}

void _glReadPixels( int x,int y,int width,int height,int format,int type,BBDataBuffer *pixels ){
	glReadPixels( x,y,width,height,format,type,pixels->WritePointer() );
}

void _glFrustumf( float left,float right,float bottom,float top,float nearVal,float farVal ){
	glFrustum( left,right,bottom,top,nearVal,farVal );
}

void _glOrthof( float left,float right,float bottom,float top,float nearVal,float farVal ){
	glOrtho( left,right,bottom,top,nearVal,farVal );
}

void _glClearDepthf( float depth  ){
	glClearDepth( depth );
}

void _glDepthRangef( float nearVal,float farVal ){
	glDepthRange( nearVal,farVal );
}
