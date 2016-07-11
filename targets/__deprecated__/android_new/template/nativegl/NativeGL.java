
package com.monkey;

public class NativeGL{

	static{
		System.loadLibrary( "nativegl" );
    }	

	public native static void glDrawElements( int mode,int count,int type,int offset );
	
	public native static void glVertexAttribPointer( int index,int size,int type,boolean normalized,int stride,int offset );
}
