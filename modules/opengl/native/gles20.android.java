
class bb_opengl_gles20{

	static boolean inited;
	static Object[] args4=new Object[4];
	static Object[] args6=new Object[6];
	static Method drawElements;
	static Method vertexAttribPointer;
	
	static Method getActiveUniform;
	static IntBuffer sizeBuf;
	static IntBuffer typeBuf;
	
	static void initNativeGL(){
	
		if( inited ) return;
		inited=true;
		
		Class c=null;
		
		try{
			c=Class.forName( "com.cerberus.NativeGL" );
		}catch( ClassNotFoundException ex ){
			c=GLES20.class;
		}

		try{
			Class[] p=new Class[]{ Integer.TYPE,Integer.TYPE,Integer.TYPE,Integer.TYPE };
			drawElements=c.getMethod( "glDrawElements",p );
		}catch( NoSuchMethodException ex ){
		}

		try{
			Class[] p=new Class[]{ Integer.TYPE,Integer.TYPE,Integer.TYPE,Boolean.TYPE,Integer.TYPE,Integer.TYPE };
			vertexAttribPointer=c.getMethod( "glVertexAttribPointer",p );
		}catch( NoSuchMethodException ex ){
		}
		
		try{
			Class[] p=new Class[]{ Integer.TYPE,Integer.TYPE,IntBuffer.class,IntBuffer.class };
			getActiveUniform=GLES20.class.getMethod( "glGetActiveUniform",p );
			sizeBuf=IntBuffer.allocate( 1 );
			typeBuf=IntBuffer.allocate( 1 );
		}catch( NoSuchMethodException ex ){
		}
	}

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
	
	static Bitmap LoadStaticTexImage( String path,int[] info ){
		Bitmap bitmap=null;
		try{
			bitmap=BBAndroidGame.AndroidGame().LoadBitmap( path );
		}catch( OutOfMemoryError e ){
			throw new Error( "Out of memory error loading bitmap" );
		}
		if( bitmap==null ) return null;
		
		if( info.length>0 ) info[0]=bitmap.getWidth();
		if( info.length>1 ) info[1]=bitmap.getHeight();
		
		return bitmap;
	}
	
	static void _glTexImage2D( int target,int level,int internalformat,int width,int height,int border,int format,int type,BBDataBuffer data ){
		GLES20.glTexImage2D( target,level,internalformat,width,height,border,format,type,data!=null ? data._data : null );
	}
	
	static void _glTexImage2D2( int target,int level,int internalFormat,int format,int type,Object data ){
		Bitmap bitmap=(Bitmap)data;
		if( bitmap!=null ) GLUtils.texImage2D( target,level,bitmap,0 );
	}

	static void _glTexSubImage2D( int target,int level,int xoffset,int yoffset,int width,int height,int format,int type,BBDataBuffer data,int dataOffset ){
		if( dataOffset==0 ){
			GLES20.glTexSubImage2D( target,level,xoffset,yoffset,width,height,format,type,data._data );
		}else{
			ByteBuffer buf=data._data;
			buf.position( dataOffset );
			GLES20.glTexSubImage2D( target,level,xoffset,yoffset,width,height,format,type,buf );
			buf.rewind();
		}
	}

	static void _glTexSubImage2D2( int target,int level,int xoffset,int yoffset,Object data ){
		Bitmap bitmap=(Bitmap)data;
		if( bitmap!=null ) GLUtils.texSubImage2D( target,level,xoffset,yoffset,bitmap );
	}
	
	static void _glBufferData( int target,int size,BBDataBuffer data,int usage ){
		GLES20.glBufferData( target,size,data!=null ? data._data : null,usage );
	}
	
	static void _glBufferSubData( int target,int offset,int size,BBDataBuffer data,int dataOffset ){
		if( dataOffset==0 ){
			GLES20.glBufferSubData( target,offset,size,data._data );
		}else{
			ByteBuffer buf=data._data;
			buf.position( dataOffset );
			GLES20.glBufferSubData( target,offset,size,buf );
			buf.rewind();
		}
	}
	
	static int _glCreateBuffer(){
		int[] tmp={0};
		GLES20.glGenBuffers( 1,tmp,0 );
		return tmp[0];
	}
	
	static int _glCreateFramebuffer(){
		int[] tmp={0};
		GLES20.glGenFramebuffers( 1,tmp,0 );
		return tmp[0];
	}
	
	static int _glCreateRenderbuffer(){
		int[] tmp={0};
		GLES20.glGenRenderbuffers( 1,tmp,0 );
		return tmp[0];
	}
	
	static int _glCreateTexture(){
		int[] tmp={0};
		GLES20.glGenTextures( 1,tmp,0 );
		return tmp[0];
	}
	
	static void _glDeleteBuffer( int buffer ){
		int[] tmp={buffer};
		GLES20.glDeleteBuffers( 1,tmp,0 );
	}
	
	static void _glDeleteFramebuffer( int buffer ){
		int[] tmp={buffer};
		GLES20.glDeleteFramebuffers( 1,tmp,0 );
	}
	
	static void _glDeleteRenderbuffer( int buffer ){
		int[] tmp={buffer};
		GLES20.glDeleteRenderbuffers( 1,tmp,0 );
	}
	
	static void _glDeleteTexture( int texture ){
		int[] tmp={texture};
		GLES20.glDeleteTextures( 1,tmp,0 );
	}

	static void _glDrawElements( int mode, int count, int type,BBDataBuffer data,int dataOffset ){
		if( dataOffset==0 ){
			GLES20.glDrawElements( mode,count,type,data._data );
		}else{
			ByteBuffer buf=data._data;
			buf.position( dataOffset );
			GLES20.glDrawElements( mode,count,type,buf );
			buf.rewind();
		}
	}
	
	static void _glDrawElements( int mode, int count, int type, int offset ){
		initNativeGL();
		args4[0]=Integer.valueOf( mode );
		args4[1]=Integer.valueOf( count );
		args4[2]=Integer.valueOf( type );
		args4[3]=Integer.valueOf( offset );
		try{
			drawElements.invoke( null,args4 );
		}catch( Exception ex ){
		}
	}
	
	static void _glGetActiveAttrib( int program, int index, int[] size, int[] type, String[] name ){
		int[] tmp={0,0,0};
		byte[] namebuf=new byte[1024];
		GLES20.glGetActiveAttrib( program,index,1024,tmp,0,tmp,1,tmp,2,namebuf,0 );
		if( size!=null && size.length!=0 ) size[0]=tmp[1];
		if( type!=null && type.length!=0 ) type[0]=tmp[2];
		if( name!=null && name.length!=0 ) name[0]=new String( namebuf,0,tmp[0] );
	}
	
	static void _glGetActiveUniform( int program, int index, int[] size,int[] type,String[] name ){
	
		int[] tmp={0,0,0};
		byte[] namebuf=new byte[1024];
		
		GLES20.glGetActiveUniform( program,index,1024,tmp,0,tmp,1,tmp,2,namebuf,0 );
		
		//work around for big ugly android 6.0 bug...
		//
		if( (tmp[1]==0 || tmp[2]==0) && getActiveUniform!=null ){
		
			try{
			
				Object args[]=new Object[]{ Integer.valueOf( program ),Integer.valueOf( index ),sizeBuf,typeBuf };

				getActiveUniform.invoke( null,args );
				
				tmp[1]=sizeBuf.get( 0 );
				tmp[2]=typeBuf.get( 0 );
				
			}catch( Exception ex ){
			}
		}
		
		if( size!=null && size.length!=0 ) size[0]=tmp[1];
		if( type!=null && type.length!=0 ) type[0]=tmp[2];
		if( name!=null && name.length!=0 ) name[0]=new String( namebuf,0,tmp[0] );
	}
	
	static void _glGetAttachedShaders( int program, int maxcount, int[] count, int[] shaders ){
		int[] cnt={0};
		int[] shdrs=new int[maxcount];
		GLES20.glGetAttachedShaders( program,maxcount,cnt,0,shdrs,0 );
		if( count!=null && count.length!=0 ) count[0]=cnt[0];
		if( shaders!=null && shaders.length!=0 ){
			int n=cnt[0];
			if( maxcount<n ) n=maxcount;
			if( shaders.length<n ) n=shaders.length;
			for( int i=0;i<n;++i ){
				shaders[i]=shdrs[i];
			}
		}
	}
	
	static void _glGetBooleanv( int pname, boolean[] params ){
		GLES20.glGetBooleanv( pname,params,0 );
	}
	
	static void _glGetBufferParameteriv( int target, int pname, int[] params ){
		GLES20.glGetBufferParameteriv( target,pname,params,0 );
	}
	
	static void _glGetFloatv( int pname,float[] params ){
		GLES20.glGetFloatv( pname,params,0 );
	}
	
	static void _glGetFramebufferAttachmentParameteriv( int target, int attachment, int pname, int[] params ){
		GLES20.glGetFramebufferAttachmentParameteriv( target,attachment,pname,params,0 );
	}
	
	static void _glGetIntegerv( int pname,int[] params ){
		GLES20.glGetIntegerv( pname,params,0 );
	}
	
	static void _glGetProgramiv( int program,int pname,int[] params ){
		GLES20.glGetProgramiv( program,pname,params,0 );
	}
	
	static void _glGetRenderbufferParameteriv( int target,int pname,int[] params ){
		GLES20.glGetRenderbufferParameteriv( target,pname,params,0 );
	}
	
	static void _glGetShaderiv( int shader, int pname, int[] params ){
		GLES20.glGetShaderiv( shader,pname,params,0 );
	}
	
	static String _glGetShaderSource( int shader ){
		int[] len={0};
		byte[] buf=new byte[1024];
		GLES20.glGetShaderSource( shader,1024,len,0,buf,0 );
		return new String( buf,0,len[0] );
	}
	
	static void _glGetTexParameterfv( int target,int pname,float[] params ){
		GLES20.glGetTexParameterfv( target,pname,params,0 );
	}
	
	static void _glGetTexParameteriv( int target,int pname,int[] params ){
		GLES20.glGetTexParameteriv( target,pname,params,0 );
	}
	
	static void _glGetUniformfv( int program, int location, float[] params ){
		GLES20.glGetUniformfv( program,location,params,0 );
	}
	
	static void _glGetUniformiv( int program, int location, int[] params ){
		GLES20.glGetUniformiv( program,location,params,0 );
	}
	
	static void _glGetVertexAttribfv( int index, int pname, float[] params ){
		GLES20.glGetVertexAttribfv( index,pname,params,0 );
	}
	
	static void _glGetVertexAttribiv( int index, int pname, int[] params ){
		GLES20.glGetVertexAttribiv( index,pname,params,0 );
	}
	
	static void _glReadPixels( int x,int y,int width,int height,int format,int type,BBDataBuffer data,int dataOffset ){
		if( dataOffset==0 ){
			GLES20.glReadPixels( x,y,width,height,format,type,data._data );
		}else{
			//another day, another android BUG?
			//
			//glReadPixels doesn't seem to work if you read into a buffer with non-0 position.
			//
			ByteBuffer tmp=ByteBuffer.allocate( width*height*4 );
			GLES20.glReadPixels( x,y,width,height,format,type,tmp );
			ByteBuffer buf=data._data;
			buf.position( dataOffset );
			buf.put( tmp );
			buf.rewind();
			
			/*
			ByteBuffer buf=data._data;
			buf.position( dataOffset );	//crash unless dataOffset is 0
			GLES20.glReadPixels( x,y,width,height,format,type,buf );
			buf.rewind();
			*/
		}
	}

	static void _glUniform1fv( int location, int count, float[] v ){
		GLES20.glUniform1fv( location,count,v,0 );
	}
	
	static void _glUniform1iv( int location, int count, int[] v ){
		GLES20.glUniform1iv( location,count,v,0 );
	}
	
	static void _glUniform2fv( int location, int count, float[] v ){
		GLES20.glUniform2fv( location,count,v,0 );
	}
	
	static void _glUniform2iv( int location, int count, int[] v ){
		GLES20.glUniform2iv( location,count,v,0 );
	}
	
	static void _glUniform3fv( int location, int count, float[] v ){
		GLES20.glUniform3fv( location,count,v,0 );
	}
	
	static void _glUniform3iv( int location, int count, int[] v ){
		GLES20.glUniform3iv( location,count,v,0 );
	}
	
	static void _glUniform4fv( int location, int count, float[] v ){
		GLES20.glUniform4fv( location,count,v,0 );
	}
	
	static void _glUniform4iv( int location, int count, int[] v ){
		GLES20.glUniform4iv( location,count,v,0 );
	}
	
	static void _glUniformMatrix2fv( int location, int count, boolean transpose, float[] value ){
		GLES20.glUniformMatrix2fv( location,count,transpose,value,0 );
	}
	
	static void _glUniformMatrix3fv( int location, int count, boolean transpose, float[] value ){
		GLES20.glUniformMatrix3fv( location,count,transpose,value,0 );
	}
	
	static void _glUniformMatrix4fv( int location, int count, boolean transpose, float[] value ){
		GLES20.glUniformMatrix4fv( location,count,transpose,value,0 );
	}
	
	static void _glVertexAttrib1fv( int indx, float[] values ){
		GLES20.glVertexAttrib1fv( indx,values,0 );
	}
	
	static void _glVertexAttrib2fv( int indx, float[] values ){
		GLES20.glVertexAttrib2fv( indx,values,0 );
	}
	
	static void _glVertexAttrib3fv( int indx, float[] values ){
		GLES20.glVertexAttrib3fv( indx,values,0 );
	}
	
	static void _glVertexAttrib4fv( int indx, float[] values ){
		GLES20.glVertexAttrib4fv( indx,values,0 );
	}
	
	static void _glVertexAttribPointer( int indx, int size, int type, boolean normalized, int stride, BBDataBuffer data, int dataOffset ){
		if( dataOffset==0 ){
			GLES20.glVertexAttribPointer( indx,size,type,normalized,stride,data._data );
		}else{
			ByteBuffer buf=data._data;
			buf.position( dataOffset );
			GLES20.glVertexAttribPointer( indx,size,type,normalized,stride,buf );
			buf.rewind();
		}
	}
	
	static void _glVertexAttribPointer( int indx, int size, int type, boolean normalized, int stride, int offset ){
		initNativeGL();
		args6[0]=Integer.valueOf( indx );
		args6[1]=Integer.valueOf( size );
		args6[2]=Integer.valueOf( type );
		args6[3]=Boolean.valueOf( normalized );
		args6[4]=Integer.valueOf( stride );
		args6[5]=Integer.valueOf( offset );
		try{
			vertexAttribPointer.invoke( null,args6 );
		}catch( Exception ex ){
		}
	}
}
