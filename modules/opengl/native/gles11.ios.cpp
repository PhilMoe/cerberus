
// Ok, a bit freaky coz we want non-premultipied alpha!
//
BBDataBuffer *BBLoadImageData( BBDataBuffer *buf,String path,Array<int> info ){

	path=String( "data/" )+path;
	NSString *nspath=path.ToNSString();

	//This was apparently buggy in iOS2.x, but NO MORE?
	UIImage *uiimage=[ UIImage imageNamed:nspath ];
	if( !uiimage ) return 0;
	
	CGImageRef cgimage=uiimage.CGImage;
	
	int width=CGImageGetWidth( cgimage );
	int height=CGImageGetHeight( cgimage );
	int pitch=CGImageGetBytesPerRow( cgimage );
	int bpp=CGImageGetBitsPerPixel( cgimage );
	
	if( bpp!=24 && bpp!=32 ) return 0;
	
	CFDataRef cfdata=CGDataProviderCopyData( CGImageGetDataProvider( cgimage ) );
	unsigned char *src=(unsigned char*)CFDataGetBytePtr( cfdata );
	int srclen=(int)CFDataGetLength( cfdata );
	
	if( !buf->_New( width*height*4 ) ) return 0;

	unsigned char *dst=(unsigned char*)buf->WritePointer();

	int y;
		
	switch( bpp ){
	case 24:
		for( y=0;y<height;++y ){
			for( int x=0;x<width;++x ){
				*dst++=*src++;
				*dst++=*src++;
				*dst++=*src++;
				*dst++=255;
			}
			src+=pitch-width*3;
		}
		break;
	case 32:
		for( y=0;y<height;++y ){
			memcpy( dst,src,width*4 );
			dst+=width*4;
			src+=pitch;
		}
		break;
	}
	
	if( info.Length()>0 ) info[0]=width;
	if( info.Length()>1 ) info[1]=height;
	
	CFRelease( cfdata );
	
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

void _glGetBufferParameteriv( int target,int pname,Array<int> params,int offset ){
	glGetBufferParameteriv( target,pname,&params[offset] );
}

void _glClipPlanef( int plane,Array<Float> equation,int offset ){
	glClipPlanef( plane,&equation[offset] );
}

void _glGetClipPlanef( int plane,Array<Float> equation,int offset ){
	glGetClipPlanef( plane,&equation[offset] );
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

