
#include <GLES2/gl2.h>

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

void _glTexImage2D( int target,int level,int internalformat,int width,int height,int border,int format,int type,BBDataBuffer *data ){
	glTexImage2D( target,level,internalformat,width,height,border,format,type,data ? data->ReadPointer() : 0 );
}

void _glTexSubImage2D( int target,int level,int xoffset,int yoffset,int width,int height,int format,int type,BBDataBuffer *data,int dataOffset ){
	glTexSubImage2D( target,level,xoffset,yoffset,width,height,format,type,data->ReadPointer( dataOffset ) );
}

void _glBindAttribLocation( int program, int index, String name ){
	glBindAttribLocation( program,(GLuint)index,name.ToCString<char>() );
}

void _glBufferData( int target,int size,BBDataBuffer *data,int usage ){
	glBufferData( target,size,data ? data->ReadPointer() : 0,usage );
}

void _glBufferSubData( int target,int offset,int size,BBDataBuffer *data,int dataOffset ){
	glBufferSubData( target,offset,size,data->ReadPointer( dataOffset ) );
}

int _glCreateBuffer(){
	GLuint buf;
	glGenBuffers( 1,&buf );
	return buf;
}

int _glCreateFramebuffer(){
	GLuint buf;
	glGenFramebuffers( 1,&buf );
	return buf;
}

int _glCreateRenderbuffer(){
	GLuint buf;
	glGenRenderbuffers( 1,&buf );
	return buf;
}

int _glCreateTexture(){
	GLuint buf;
	glGenTextures( 1,&buf );
	return buf;
}

void _glDeleteBuffer( int buffer ){
	glDeleteBuffers( 1,(GLuint*)&buffer );
}

void _glDeleteFramebuffer( int buffer ){
	glDeleteFramebuffers( 1,(GLuint*)&buffer );
}

void _glDeleteRenderbuffer( int buffer ){
	glDeleteRenderbuffers( 1,(GLuint*)&buffer );
}

void _glDeleteTexture( int texture ){
	glDeleteTextures( 1,(GLuint*)&texture );
}

void _glDrawElements( int mode, int count, int type, BBDataBuffer *data,int dataOffset ){
	glDrawElements( mode,count,type,data->ReadPointer( dataOffset ) );
}

void _glDrawElements( int mode, int count, int type, int offset ){
	glDrawElements( mode,count,type,(const GLvoid*)offset );
}

void _glGetActiveAttrib( int program, int index, Array<int> size,Array<int> type,Array<String> name ){
	int len=0,ty=0,sz=0;char nm[1024];
	glGetActiveAttrib( program,index,1024,&len,&sz,(GLenum*)&ty,nm );
	nm[1023]=0;
	if( size.Length() ) size[0]=sz;
	if( type.Length() ) type[0]=ty;
	if( name.Length() ) name[0]=String( nm );
}

void _glGetActiveUniform( int program, int index, Array<int> size,Array<int> type,Array<String> name ){
	int len=0,ty=0,sz=0;char nm[1024];
	glGetActiveUniform( program,index,1024,&len,&sz,(GLenum*)&ty,nm );
	nm[1023]=0;
	if( size.Length() ) size[0]=sz;
	if( type.Length() ) type[0]=ty;
	if( name.Length() ) name[0]=String( nm );
}

void _glGetAttachedShaders( int program, int maxcount, Array<int> count, Array<int> shaders ){
	int cnt=0,sh[32];
	glGetAttachedShaders( program,32,&cnt,(GLuint*)sh );
	if( count.Length() ) count[0]=cnt;
	if( shaders.Length() ){
		int n=cnt;
		if( maxcount<n ) n=maxcount;
		if( shaders.Length()<n ) n=shaders.Length();
		for( int i=0;i<n;++i ){
			shaders[i]=sh[i];
		}
	}
}

int _glGetAttribLocation( int program, String name ){
	return glGetAttribLocation( program,name.ToCString<char>() );
}

void _glGetBooleanv( int pname, Array<bool> params ){
	if( sizeof(bool)!=1 ){
		puts( "sizeof(bool) error in gles20.glfw.cpp!" );
		return;
	}
	glGetBooleanv( pname,(GLboolean*)&params[0] );
}

void _glGetBufferParameteriv( int target, int pname, Array<int> params ){
	glGetBufferParameteriv( target,pname,&params[0] );
}

void _glGetFloatv( int pname,Array<Float> params ){
	glGetFloatv( pname,&params[0] );
}

void _glGetFramebufferAttachmentParameteriv( int target, int attachment, int pname, Array<int> params ){
	glGetFramebufferAttachmentParameteriv( target,attachment,pname,&params[0] );
}

void _glGetIntegerv( int pname,Array<int> params ){
	glGetIntegerv( pname,&params[0] );
}

void _glGetProgramiv( int program,int pname,Array<int> params ){
	glGetProgramiv( program,pname,&params[0] );
}

String _glGetProgramInfoLog( int program ){
	int length=0,length2=0;
	glGetProgramiv( program,GL_INFO_LOG_LENGTH,&length );
	char *buf=(char*)malloc( length+1 );
	glGetProgramInfoLog( program,length,&length2,buf );
	String t=String( buf );
	free( buf );
	return t;
}

void _glGetRenderbufferParameteriv( int target,int pname,Array<int> params ){
	glGetRenderbufferParameteriv( target,pname,&params[0] );
}

void _glGetShaderiv( int shader, int pname, Array<int> params ){
	glGetShaderiv( shader,pname,&params[0] );
}

String _glGetShaderInfoLog( int shader ){
	int length=0,length2=0;
	glGetShaderiv( shader,GL_INFO_LOG_LENGTH,&length );
	char *buf=(char*)malloc( length+1 );
	glGetShaderInfoLog( shader,length,&length2,buf );
	String t=String( buf );
	free( buf );
	return t;
}

String _glGetShaderSource( int shader ){
	int length=0,length2=0;
	glGetShaderiv( shader,GL_SHADER_SOURCE_LENGTH,&length );
	char *buf=(char*)malloc( length+1 );
	glGetShaderSource( shader,length,&length2,buf );
	String t=String( buf );
	free( buf );
	return t;
}

String _glGetString( int name ){
	return String( glGetString( name ) );
}

void _glGetTexParameterfv( int target,int pname,Array<float> params ){
	glGetTexParameterfv( target,pname,&params[0] );
}

void _glGetTexParameteriv( int target,int pname,Array<int> params ){
	glGetTexParameteriv( target,pname,&params[0] );
}

void _glGetUniformfv( int program, int location, Array<float> params ){
	glGetUniformfv( program,location,&params[0] );
}

void _glGetUniformiv( int program, int location, Array<int> params ){
	glGetUniformiv( program,location,&params[0] );
}

int _glGetUniformLocation( int program, String name ){
	return glGetUniformLocation( program,name.ToCString<char>() );
}

void _glGetVertexAttribfv( int index, int pname, Array<float> params ){
	glGetVertexAttribfv( index,pname,&params[0] );
}

void _glGetVertexAttribiv( int index, int pname, Array<int> params ){
	glGetVertexAttribiv( index,pname,&params[0] );
}

void _glReadPixels( int x,int y,int width,int height,int format,int type,BBDataBuffer *data,int dataOffset ){
	glReadPixels( x,y,width,height,format,type,data->WritePointer( dataOffset ) );
}

void _glShaderSource( int shader, String source ){
	String::CString<char> cstr=source.ToCString<char>();
	const char *buf[1];
	buf[0]=cstr;
	glShaderSource( shader,1,(const GLchar**)buf,0 );
}

void _glUniform1fv( int location, int count, Array<float> v ){
	glUniform1fv( location,count,&v[0] );
}

void _glUniform1iv( int location, int count, Array<int> v ){
	glUniform1iv( location,count,&v[0] );
}

void _glUniform2fv( int location, int count, Array<float> v ){
	glUniform2fv( location,count,&v[0] );
}

void _glUniform2iv( int location, int count, Array<int> v ){
	glUniform2iv( location,count,&v[0] );
}

void _glUniform3fv( int location, int count, Array<float> v ){
	glUniform3fv( location,count,&v[0] );
}

void _glUniform3iv( int location, int count, Array<int> v ){
	glUniform3iv( location,count,&v[0] );
}

void _glUniform4fv( int location, int count, Array<float> v ){
	glUniform4fv( location,count,&v[0] );
}

void _glUniform4iv( int location, int count, Array<int> v ){
	glUniform4iv( location,count,&v[0] );
}

void _glUniformMatrix2fv( int location, int count, bool transpose, Array<float> value ){
	glUniformMatrix2fv( location,count,transpose,&value[0] );
}

void _glUniformMatrix3fv( int location, int count, bool transpose, Array<float> value ){
	glUniformMatrix3fv( location,count,transpose,&value[0] );
}

void _glUniformMatrix4fv( int location, int count, bool transpose, Array<float> value ){
	glUniformMatrix4fv( location,count,transpose,&value[0] );
}

void _glVertexAttrib1fv( int indx, Array<float> values ){
	glVertexAttrib1fv( indx,&values[0] );
}

void _glVertexAttrib2fv( int indx, Array<float> values ){
	glVertexAttrib2fv( indx,&values[0] );
}

void _glVertexAttrib3fv( int indx, Array<float> values ){
	glVertexAttrib3fv( indx,&values[0] );
}

void _glVertexAttrib4fv( int indx, Array<float> values ){
	glVertexAttrib4fv( indx,&values[0] );
}

void _glVertexAttribPointer( int indx, int size, int type, bool normalized, int stride, BBDataBuffer *data, int dataOffset ){
	glVertexAttribPointer( indx,size,type,normalized,stride,data->ReadPointer( dataOffset ) );
}

void _glVertexAttribPointer( int indx, int size, int type, bool normalized, int stride, int offset ){
	glVertexAttribPointer( indx,size,type,normalized,stride,(const GLvoid*)offset );
}
