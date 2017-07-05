
#include <jni.h>
#include <stdlib.h>

jint Java_com_cerberus_LangUtil_parseInt( JNIEnv *env,jclass c,jstring str ){

	int i,len;
	char buf[64];
	jchar jbuf[64];

	len=(*env)->GetStringLength( env,str );
	if( len>63 ) len=63;
	
	(*env)->GetStringRegion( env,str,0,len,jbuf );
	for( i=0;i<len;++i ) buf[i]=jbuf[i];
	buf[len]=0;
	
	return atoi( buf );
}

jfloat Java_com_cerberus_LangUtil_parseFloat( JNIEnv *env,jclass c,jstring str ){

	int i,len;
	char buf[128];
	jchar jbuf[128];

	len=(*env)->GetStringLength( env,str );
	if( len>127 ) len=127;
	
	(*env)->GetStringRegion( env,str,0,len,jbuf );
	for( i=0;i<len;++i ) buf[i]=jbuf[i];
	buf[len]=0;
	
	return atof( buf );
}
