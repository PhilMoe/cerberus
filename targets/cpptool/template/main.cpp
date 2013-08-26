
#include "main.h"

//${CONFIG_BEGIN}
//${CONFIG_END}

//${TRANSCODE_BEGIN}
//${TRANSCODE_END}

String BBPathToFilePath( String path ){
	return path;
}

int main( int argc,const char **argv ){

	new BBGame();

	try{
	
		bb_std_main( argc,argv );
		
	}catch( ThrowableObject *ex ){
	
		bbPrint( "Monkey Runtime Error : Uncaught Monkey Exception" );
	
	}catch( const char *err ){
	
	}
}
