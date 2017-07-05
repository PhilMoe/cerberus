
package com.cerberus;

public class LangUtil{

	static{
		System.loadLibrary( "langutil" );
    }	

	public native static int parseInt( String str );
	
	public native static float parseFloat( String str );
}
