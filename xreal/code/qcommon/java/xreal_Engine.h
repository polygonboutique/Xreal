/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class xreal_Engine */

#ifndef _Included_xreal_Engine
#define _Included_xreal_Engine
#ifdef __cplusplus
extern "C" {
#endif
#undef xreal_Engine_MAX_CLIENTS
#define xreal_Engine_MAX_CLIENTS 64L
#undef xreal_Engine_MAX_LOCATIONS
#define xreal_Engine_MAX_LOCATIONS 64L
#undef xreal_Engine_GENTITYNUM_BITS
#define xreal_Engine_GENTITYNUM_BITS 11L
#undef xreal_Engine_MAX_GENTITIES
#define xreal_Engine_MAX_GENTITIES 2048L
#undef xreal_Engine_ENTITYNUM_NONE
#define xreal_Engine_ENTITYNUM_NONE 2047L
#undef xreal_Engine_ENTITYNUM_WORLD
#define xreal_Engine_ENTITYNUM_WORLD 2046L
#undef xreal_Engine_ENTITYNUM_MAX_NORMAL
#define xreal_Engine_ENTITYNUM_MAX_NORMAL 2046L
#undef xreal_Engine_GMODELNUM_BITS
#define xreal_Engine_GMODELNUM_BITS 8L
#undef xreal_Engine_MAX_MODELS
#define xreal_Engine_MAX_MODELS 256L
#undef xreal_Engine_MAX_SOUNDS
#define xreal_Engine_MAX_SOUNDS 256L
#undef xreal_Engine_MAX_EFFECTS
#define xreal_Engine_MAX_EFFECTS 256L
#undef xreal_Engine_MAX_CONFIGSTRINGS
#define xreal_Engine_MAX_CONFIGSTRINGS 1024L
#undef xreal_Engine_EXEC_NOW
#define xreal_Engine_EXEC_NOW 0L
#undef xreal_Engine_EXEC_INSERT
#define xreal_Engine_EXEC_INSERT 1L
#undef xreal_Engine_EXEC_APPEND
#define xreal_Engine_EXEC_APPEND 2L
/*
 * Class:     xreal_Engine
 * Method:    print
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_xreal_Engine_print
  (JNIEnv *, jclass, jstring);

/*
 * Class:     xreal_Engine
 * Method:    error
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_xreal_Engine_error
  (JNIEnv *, jclass, jstring);

/*
 * Class:     xreal_Engine
 * Method:    getTimeInMilliseconds
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_xreal_Engine_getTimeInMilliseconds
  (JNIEnv *, jclass);

/*
 * Class:     xreal_Engine
 * Method:    getConsoleArgc
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_xreal_Engine_getConsoleArgc
  (JNIEnv *, jclass);

/*
 * Class:     xreal_Engine
 * Method:    getConsoleArgv
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_xreal_Engine_getConsoleArgv
  (JNIEnv *, jclass, jint);

/*
 * Class:     xreal_Engine
 * Method:    getConsoleArgs
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_xreal_Engine_getConsoleArgs
  (JNIEnv *, jclass);

/*
 * Class:     xreal_Engine
 * Method:    sendConsoleCommand
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_xreal_Engine_sendConsoleCommand
  (JNIEnv *, jclass, jint, jstring);

/*
 * Class:     xreal_Engine
 * Method:    readFile
 * Signature: (Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_xreal_Engine_readFile
  (JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif
#endif
