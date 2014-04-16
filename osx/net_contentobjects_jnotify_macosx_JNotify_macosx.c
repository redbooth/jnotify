#include "net_contentobjects_jnotify_macosx_JNotify_macosx.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

struct listnode;

struct listnode
{
  FSEventStreamRef stream;
  jint id;
  struct listnode *next;
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
  (void)streamRef;
  (void)eventIds;
  const char **cpaths = (const char **) eventPaths;
  struct listnode *node = (struct listnode *) userData;
  size_t i;

  // start batch
  (*loopEnv)->CallVoidMethod(loopEnv, loopClass, loopMethodInBatch, node->id, JNI_TRUE);

  for (i = 0; i < numEvents; i++)
  {
    size_t len = strlen(cpaths[i]);
    jbyteArray jpath = (*loopEnv)->NewByteArray(loopEnv, len);
    (*loopEnv)->SetByteArrayRegion(loopEnv, jpath, 0, len, cpaths[i]);

    // notify
    (*loopEnv)->CallVoidMethod(loopEnv, loopClass, loopMethodProcessEvent, node->id, jpath, (jint)eventFlags[i]);
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

  loopMethodProcessEvent = (*env)->GetStaticMethodID(env, clazz, "callbackProcessEvent", "(I[BI)V");
  if (loopMethodProcessEvent == NULL)
    return;

  loopMethodInBatch = (*env)->GetStaticMethodID(env, clazz, "callbackInBatch", "(IZ)V");
  if (loopMethodInBatch == NULL)
    return;
}

JNIEXPORT jint JNICALL Java_net_contentobjects_jnotify_macosx_JNotify_1macosx_nativeAddWatch
  (JNIEnv *env, jclass clazz, jstring jpath)
{
  (void)clazz;
  jint id = 0;

  // get the path as a CFString
  const jchar *path = (*env)->GetStringChars(env, jpath, NULL);
  if (path == NULL)
    return 0;
  CFStringRef cfpath = CFStringCreateWithCharacters(NULL, path, (*env)->GetStringLength(env, jpath));
  (*env)->ReleaseStringChars(env, jpath, path);
  if (cfpath == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/OutOfMemoryError"), "Could not allocate CFString");
    goto nostring;
  }

  // make an array
  CFArrayRef paths = CFArrayCreate(NULL, (void *) &cfpath, 1, NULL);
  if (paths == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/OutOfMemoryError"), "Could not allocate CFArray");
    goto noarray;
  }

  // create the listnode
  struct listnode *node = malloc(sizeof(struct listnode));
  if (node == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/OutOfMemoryError"), "Could not allocate listnode");
    goto nonode;
  }

  // create the stream
  FSEventStreamContext context = {0, node, NULL, NULL, NULL};
  node->stream = FSEventStreamCreate(NULL, &fsevent_callback, &context, paths, kFSEventStreamEventIdSinceNow, 1, kFSEventStreamCreateFlagIgnoreSelf);
  if (node->stream == NULL)
  {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "net/contentobjects/jnotify/macosx/JNotifyException_macosx"), "Could not create stream");
    free(node);
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
    goto nonode;
  }

  // put the stream in the list
  struct listnode **cursor = &streams;
  while (*cursor != NULL && (*cursor)->id == id) {
    cursor = &(*cursor)->next;
    id++;
  }
  node->id = id;
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
  (void)env;
  (void)clazz;
  struct listnode **node = &streams;
  struct listnode *deletedNode = NULL;

  while (*node != NULL && (*node)->id < wd)
    node = &(*node)->next;

  if ((*node)->id == wd)
  {
    deletedNode = *node;
    *node = (*node)->next;
    FSEventStreamStop(deletedNode->stream);
    FSEventStreamInvalidate(deletedNode->stream);
    FSEventStreamRelease(deletedNode->stream);
    free(deletedNode);
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

