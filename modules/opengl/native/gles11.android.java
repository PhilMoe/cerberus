
class bb_opengl_gles11{

	static BBDataBuffer LoadImageData( BBDataBuffer buf,String path,int[] info ){
		Bitmap bitmap=null;
		try{
			bitmap=BBAndroidGame.AndroidGame().LoadBitmap( path );
		}catch( OutOfMemoryError e ){
			throw new Error( "Out of memory error loading bitmap" );
		}
		if( bitmap==null ) return null;
		
		int width=bitmap.getWidth(),height=bitmap.getHeight();
	
		int size=width*height;
		int[] pixels=new int[size];
		bitmap.getPixels( pixels,0,width,0,0,width,height );
		
		if( !buf._New( size*4 ) ) return null;
		
		for( int i=0;i<size;++i ){
			int p=pixels[i];
			int a=(p>>24) & 255;
			int r=(p>>16) & 255;
			int g=(p>>8) & 255;
			int b=p & 255;
			buf.PokeInt( i*4,(a<<24)|(b<<16)|(g<<8)|r );
		}
		
		if( info.length>0 ) info[0]=width;
		if( info.length>1 ) info[1]=height;
		
		return buf;
	}

	static void _glVertexPointer( int size,int type,int stride,BBDataBuffer pointer ){
		GLES11.glVertexPointer( size,type,stride,pointer._data );
	}
	
	static void _glColorPointer( int size,int type,int stride,BBDataBuffer pointer ){
		GLES11.glColorPointer( size,type,stride,pointer._data );
	}
	
	static void _glNormalPointer( int type,int stride,BBDataBuffer pointer ){
		GLES11.glNormalPointer( type,stride,pointer._data );
	}
	
	static void _glTexCoordPointer( int size,int type,int stride,BBDataBuffer pointer ){
		GLES11.glTexCoordPointer( size,type,stride,pointer._data );
	}
	
	static void _glDrawElements( int mode,int count,int type,BBDataBuffer indices ){
		GLES11.glDrawElements( mode,count,type,indices._data );
	}
	
	static void _glBufferData( int target,int size,BBDataBuffer data,int usage ){
		GLES11.glBufferData( target,size,data._data,usage );
	}
	
	static void _glBufferSubData( int target,int offset,int size,BBDataBuffer data ){
		GLES11.glBufferSubData( target,offset,size,data._data );
	}
	
	static void _glTexImage2D( int target,int level,int internalformat,int width,int height,int border,int format,int type,BBDataBuffer pixels ){
		GLES11.glTexImage2D( target,level,internalformat,width,height,border,format,type,pixels._data );
	}
	
	static void _glTexSubImage2D( int target,int level,int xoffset,int yoffset,int width,int height,int format,int type,BBDataBuffer pixels ){
		GLES11.glTexSubImage2D( target,level,xoffset,yoffset,width,height,format,type,pixels._data );
	}
	
	static void _glCompressedTexImage2D( int target,int level,int internalformat,int width,int height,int border,int imageSize,BBDataBuffer pixels ){
		GLES11.glCompressedTexImage2D( target,level,internalformat,width,height,border,imageSize,pixels._data );
	}
	
	static void _glCompressedTexSubImage2D( int target,int level,int xoffset,int yoffset,int width,int height,int format,int imageSize,BBDataBuffer pixels ){
		GLES11.glCompressedTexSubImage2D( target,level,xoffset,yoffset,width,height,format,imageSize,pixels._data );
	}
	
	static void _glReadPixels( int x,int y,int width,int height,int format,int type,BBDataBuffer pixels ){
		GLES11.glReadPixels( x,y,width,height,format,type,pixels._data );
	}
}

