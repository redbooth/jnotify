#include "net_contentobjects_jnotify_macosx_JNotify_macosx.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

struct listnode;

struct listnode
{
  FSEventStreamRef stream;
  jint id;
  struct listnode *next;
  jobject root;
};

static struct listnode *streams;
static JNIEnv *loopEnv;
static jclass loopClass;
static jmethodID loopMethodProcessEvent;
static jmethodID loopMethodInBatch;
static CFRunLoopRef runLoop;

static void fsevent_callback
  (ConstFSEventStreamRef streamRef, void *userData, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
  const char **cpaths = (const char **) eventPaths;
  struct listnode *node = (struct listnode *) userData;
  size_t i;
  jstring jpath;
  jboolean recursive;

  // start batch
  (*loopEnv)->CallVoidMethod(loopEnv, loopClass, loopMethodInBatch, node->id, JNI_TRUE);

  for (i = 0; i < numEvents; i++)
  {
    // these flags mean we need a full rescan
    if (eventFlags[i] & (kFSEventStreamEventFlagUserDropped | kFSEventStreamEventFlagKernelDropped))
      jpath = node->root;
    else
      jpath = (*loopEnv)->NewStringUTF(loopEnv, cpaths[i]);

    // these flags mean we need to scan recursively
    if (eventFlags[i] & (kFSEventStreamEventFlagUserDropped | kFSEventStreamEventFlagKernelDropped | kFSEventStreamEventFlagMustScanSubDirs))
      recursive = JNI_TRUE;
    else
      recursive = JNI_FALSE;

    // notify
    (*loopEnv)->CallVoidMethod(loopEnv, loopClass, loopMethodProcessEvent, node->id, node->root, jpath, recursive);
    // we need to delete this here or the JVM will hold the string until the thread exits
    (*loopEnv)->DeleteLocalRef(loopEnv, jpath);
  }

  // end batch
  (*loopEnv)->CallVoidMethod(loopEnv, loopClass, loopMethodInBatch, node->id, JNI_FALSE);
}

JNIEXPORT void JNICALL Java_net_contentobjects_jnotify_macosx_JNotify_1macosx_nativeInit
  (JNIEnv *env, jclass clazz)
{
  streams = NULL;
  loopEnv = NULL;
  runLoop = CFRunLoopGetCurrent();

  loopMethodProcessEvent = (*env)->GetStaticMethodID(env, clazz, "callbackProcessEvent", "(ILjava/lang/String;Ljava/lang/String;Z)V");
  if (loopMethodProcessEvent == NULL)
    return;

  loopMethodInBatch = (*env)->GetStaticMethodID(env, clazz, "callbackInBatch", "(IZ)V");
  if (loopMethodInBatch == NULL)
    return;
}

JNIEXPORT jint JNICALL Java_net_contentobjects_jnotify_macosx_JNotify_1macosx_nativeAddWatch
  (JNIEnv *env, jclass clazz, jstring jpath)
{
  jint id = 0;

  // we need this later
  jpath = (*env)->NewGlobalRef(env, jpath);
  if (jpath == NULL)
    return id;

  // get the path as a CFString
  const jchar *path = (*env)->GetStringChars(env, jpath, NULL);
  if (path == NULL)
    return 0;
  CFStringRef cfpath = CFStringCreateWithCharacters(NULL, path, (*env)->GetStringLength(env, jpath));
  (*env)->ReleaseStringChars(env, jpath, path);
  if (cfpath == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/OutOfMemoryError"), "Could not allocate CFString");
    (*env)->DeleteGlobalRef(env, jpath);
    goto nostring;
  }

  // make an array
  CFArrayRef paths = CFArrayCreate(NULL, (void *) &cfpath, 1, NULL);
  if (paths == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/OutOfMemoryError"), "Could not allocate CFArray");
    (*env)->DeleteGlobalRef(env, jpath);
    goto noarray;
  }

  // create the listnode
  struct listnode *node = malloc(sizeof(struct listnode));
  if (node == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/OutOfMemoryError"), "Could not allocate listnode");
    (*env)->DeleteGlobalRef(env, jpath);
    goto nonode;
  }

  // create the stream
  FSEventStreamContext context = {0, node, NULL, NULL, NULL};
  node->stream = FSEventStreamCreate(NULL, &fsevent_callback, &context, paths, kFSEventStreamEventIdSinceNow, 1, kFSEventStreamCreateFlagIgnoreSelf);
  if (node->stream == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "net/contentobjects/jnotify/macosx/JNotifyException_macosx"), "Could not create stream");
    free(node);
    (*env)->DeleteGlobalRef(env, jpath);
    goto nonode;
  }

  // start the stream
  FSEventStreamScheduleWithRunLoop(node->stream, runLoop, kCFRunLoopDefaultMode);
  if (!FSEventStreamStart(node->stream))
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "net/contentobjects/jnotify/macosx/JNotifyException_macosx"), "Could not start stream");
    FSEventStreamInvalidate(node->stream);
    FSEventStreamRelease(node->stream);
    free(node);
    (*env)->DeleteGlobalRef(env, jpath);
    goto nonode;
  }

  // put the stream in the list
  struct listnode **cursor = &streams;
  while (*cursor != NULL && (*cursor)->id == id) {
    cursor = &(*cursor)->next;
    id++;
  }
  node->id = id;
  node->root = jpath;
  node->next = *cursor;
  *cursor = node;

nonode:
  CFRelease(paths);
noarray:
  CFRelease(cfpath);
nostring:
  return id;
}

JNIEXPORT jboolean JNICALL Java_net_contentobjects_jnotify_macosx_JNotify_1macosx_nativeRemoveWatch
  (JNIEnv *env, jclass clazz, jint wd)
{
  struct listnode **node = &streams;
  struct listnode *delete = NULL;

  while (*node != NULL && (*node)->id < wd)
    node = &(*node)->next;

  if ((*node)->id == wd)
  {
    delete = *node;
    *node = (*node)->next;
    FSEventStreamStop(delete->stream);
    FSEventStreamInvalidate(delete->stream);
    FSEventStreamRelease(delete->stream);
    (*env)->DeleteGlobalRef(env, delete->root);
    free(delete);
    return JNI_TRUE;
  }
  return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_net_contentobjects_jnotify_macosx_JNotify_1macosx_nativeNotifyLoop
  (JNIEnv *env, jclass clazz)
{
  loopEnv = env;
  loopClass = clazz;
  CFRunLoopRun();
}

