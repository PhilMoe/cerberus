
#include <jni.h>
#include <GLES2/gl2.h>

void Java_com_monkey_NativeGL_glDrawElements( JNIEnv *env,jclass c,jint mode,jint count,jint type,jint offset ){
	glDrawElements( mode,count,type,(void*)offset );
}

void Java_com_monkey_NativeGL_glVertexAttribPointer( JNIEnv *env,jclass c,jint index,jint size,jint type,jboolean normalized,jint stride,jint offset ){
	glVertexAttribPointer( index,size,type,normalized,stride,(void*)offset );
}
