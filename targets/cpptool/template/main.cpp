
#include "main.h"

//${CONFIG_BEGIN}
//${CONFIG_END}

//${TRANSCODE_BEGIN}
//${TRANSCODE_END}

FILE *fopenFile( String path,String mode ){

	if( !path.StartsWith( "monkey://" ) ){
		path=path;
	}else if( path.StartsWith( "monkey://data/" ) ){
		path=String("./data/")+path.Slice(14);
	}else if( path.StartsWith( "monkey://internal/" ) ){
		path=String("./internal/")+path.Slice(18);
	}else if( path.StartsWith( "monkey://external/" ) ){
		path=String("./external/")+path.Slice(18);
	}else{
		return 0;
	}

#if _WIN32
	return _wfopen( path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() );
#else
	return fopen( path.ToCString<char>(),mode.ToCString<char>() );
#endif
}

int main( int argc,const char **argv ){

	try{
	
		bb_std_main( argc,argv );
		
	}catch( ThrowableObject *ex ){
	
		Print( "Monkey Runtime Error : Uncaught Monkey Exception" );
	
	}catch( const char *err ){
	
	}
}
