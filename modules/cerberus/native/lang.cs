
// C# Cerberus runtime.
//
// Placed into the public domain 24/02/2011.
// No warranty implied; use at your own risk.

//using System;
//using System.Collections;

public class bb_std_lang{

	public static String errInfo="";
	public static List<String> errStack=new List<String>();
	
	public const float D2R=0.017453292519943295f;
	public const float R2D=57.29577951308232f;
	
	public static void pushErr(){
		errStack.Add( errInfo );
	}
	
	public static void popErr(){
		errInfo=errStack[ errStack.Count-1 ];
		errStack.RemoveAt( errStack.Count-1 );
	}

	public static String StackTrace(){
		if( errInfo.Length==0 ) return "";
		
		String str=errInfo+"\n";
		for( int i=errStack.Count-1;i>0;--i ){
			str+=errStack[i]+"\n";
		}
		return str;
	}
	
	public static int Print( String str ){
#if WINDOWS_PHONE
		System.Diagnostics.Debug.WriteLine( str );
#else	
		Console.WriteLine( str );
#endif		
		return 0;
	}
	
	public static int Error( String str ){
		throw new Exception( str );
	}
	
	public static int DebugLog( String str ){
		Print( str );
		return 0;
	}
	
	public static int DebugStop(){
		Error( "STOP" );
		return 0;
	}
	
	/*
	public static void PrintError( String err ){
		if( err.Length==0 ) return;
		Print( "Cerberus Runtime Error : "+err );
		Print( "" );
		Print( StackTrace() );
	}
	*/
	
	//***** String stuff *****
	
	static public String[] stringArray( int n ){
		String[] t=new String[n];
		for( int i=0;i<n;++i ) t[i]="";
		return t;
	}
	
	static public String slice( String str,int from ){
		return slice( str,from,str.Length );
	}
	
	static public String slice( String str,int from,int term ){
		int len=str.Length;
		if( from<0 ){
			from+=len;
			if( from<0 ) from=0;
		}else if( from>len ){
			from=len;
		}
		if( term<0 ){
			term+=len;
		}else if( term>len ){
			term=len;
		}
		if( term>from ) return str.Substring( from,term-from );
		return "";
	}

	static public String[] split( String str,String sep ){
		if( sep.Length==0 ){
			String[] bits=new String[str.Length];
			for( int i=0;i<str.Length;++i ){
				bits[i]=new String( str[i],1 );
			}
			return bits;
		}else{
			int i=0,i2,n=1;
			while( (i2=str.IndexOf( sep,i ))!=-1 ){
				++n;
				i=i2+sep.Length;
			}
			String[] bits=new String[n];
			i=0;
			for( int j=0;j<n;++j ){
				i2=str.IndexOf( sep,i );
				if( i2==-1 ) i2=str.Length;
				bits[j]=slice( str,i,i2 );
				i=i2+sep.Length;
			}
			return bits;
		}
	}
	
	static public String fromChars( int[] chars ){
		int n=chars.Length;
		char[] chrs=new char[n];
		for( int i=0;i<n;++i ){
			chrs[i]=(char)chars[i];
		}
		return new String( chrs,0,n );
	}
	
	//***** Array stuff *****
	
	static public int length( Array arr ){
		return arr!=null ? arr.Length : 0;
	}
	
	static public Array slice( Array arr,int from ){
		if( arr==null ) return null;
		return slice( arr,from,arr.Length );
	}
	
	static public Array slice( Array arr,int from,int term ){
		if( arr==null ) return null;
		int len=arr.Length;
		if( from<0 ){
			from+=len;
			if( from<0 ) from=0;
		}else if( from>len ){
			from=len;
		}
		if( term<0 ){
			term+=len;
		}else if( term>len ){
			term=len;
		}
		if( term<from ) term=from;
		int newlen=term-from;
		Array res=Array.CreateInstance( arr.GetType().GetElementType(),newlen );
		if( newlen>0 ) Array.Copy( arr,from,res,0,newlen );
		return res;
	}
	
	static public String[] resize( String[] arr,int newlen ){
		if( arr==null ) return stringArray( newlen );
		int len=arr.Length;
		int n=Math.Min( len,newlen );
		String[] res=new String[newlen];
		if( n>0 ) Array.Copy( arr,res,n );
		while( len<newlen ) res[len++]="";
		return res;		
	}
	
	static public Array resize( Array arr,int newlen,Type elemTy ){
		Array res=Array.CreateInstance( elemTy,newlen );
		if( arr==null ) return res;
		int len=arr.Length;
		int n=Math.Min( len,newlen );
		if( n>0 ) Array.Copy( arr,res,n );
		return res;
	}

	static public int[] toChars( String str ){
		int[] arr=new int[str.Length];
		for( int i=0;i<str.Length;++i ) arr[i]=(int)str[i];
		return arr;
	}
}

class ThrowableObject : Exception{
	public ThrowableObject() : base( "Uncaught Cerberus Exception" ){
	}
};
