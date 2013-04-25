#include <jni.h>
#include <sys/stat.h>
#include <errno.h>

static jfieldID finode, fdeviceid;

// populates the deviceid and inode fields of this JNFile
static void JNICALL JNFile_stat (JNIEnv *env, jobject this, jstring jpath)
{
  const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
  if (path == NULL)
    return;
  struct stat64 stats;
  if (lstat64(path, &stats))
  {
    if (errno == ENOENT)
      (*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/FileNotFoundException"), path);
    else
      (*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"), path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    return;
  }
  (*env)->ReleaseStringUTFChars(env, jpath, path);

  (*env)->SetIntField(env, this, fdeviceid, stats.st_dev);
  (*env)->SetLongField(env, this, finode, stats.st_ino);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
  JNIEnv *env;
  jclass class;
  JNINativeMethod jnm;

  if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_2))
    return JNI_ERR;

  class = (*env)->FindClass(env, "net/contentobjects/jnotify/macosx/JNotifyAdapterMacOSX$JNFile");
  if (class == NULL)
    return JNI_ERR;

  finode = (*env)->GetFieldID(env, class, "inode", "J");
  if (finode == NULL)
    return JNI_ERR;

  fdeviceid = (*env)->GetFieldID(env, class, "deviceid", "I");
  if (fdeviceid == NULL)
    return JNI_ERR;

  // I had some trouble doing this the usual way, so I guess I'll manually
  // attach it
  jnm.name = "stat";
  jnm.signature = "(Ljava/lang/String;)V";
  jnm.fnPtr = JNFile_stat;
  (*env)->RegisterNatives(env, class, &jnm, 1);

  return JNI_VERSION_1_2;
}
