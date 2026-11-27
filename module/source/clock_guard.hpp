#pragma once
#include <jni.h>
#include <android/log.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
namespace clock_guard {
static void *bytes=nullptr;
static size_t length=0;
static bool finished=false;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
inline void prepare(int moduleDir){
 if(faccessat(moduleDir,"clock-aod-disabled",F_OK,0)==0)return;
 int fd=openat(moduleDir,"clock/clock-aod.dex",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return;
 struct stat st{};if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<8||st.st_size>131072){close(fd);return;}
 void *data=malloc((size_t)st.st_size);if(!data){close(fd);return;}
 size_t n=0;while(n<(size_t)st.st_size){ssize_t got=read(fd,(char*)data+n,(size_t)st.st_size-n);if(got<=0)break;n+=(size_t)got;}close(fd);
 if(n!=(size_t)st.st_size){free(data);return;}
 bytes=data;length=n;
}
inline void ensure(JNIEnv *e){
 if(!bytes||__atomic_load_n(&finished,__ATOMIC_ACQUIRE)||pthread_mutex_trylock(&mutex))return;
 if(__atomic_load_n(&finished,__ATOMIC_RELAXED)){pthread_mutex_unlock(&mutex);return;}
 bool retry=false;
 if(e->PushLocalFrame(24)==0){
  bool ok=[&](){
   jclass at=e->FindClass("android/app/ActivityThread");if(!at)return false;
   jmethodID current=e->GetStaticMethodID(at,"currentApplication","()Landroid/app/Application;");if(!current)return false;
   jobject app=e->CallStaticObjectMethod(at,current);if(e->ExceptionCheck())return false;if(!app){retry=true;return false;}
   jclass appClass=e->GetObjectClass(app);if(!appClass)return false;
   jmethodID getLoader=e->GetMethodID(appClass,"getClassLoader","()Ljava/lang/ClassLoader;");if(!getLoader)return false;
   jobject parent=e->CallObjectMethod(app,getLoader);if(!parent||e->ExceptionCheck())return false;
   jclass dex=e->FindClass("dalvik/system/InMemoryDexClassLoader");if(!dex)return false;
   jmethodID ctor=e->GetMethodID(dex,"<init>","(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");if(!ctor)return false;
   jobject buffer=e->NewDirectByteBuffer(bytes,(jlong)length);if(!buffer)return false;
   jobject loader=e->NewObject(dex,ctor,buffer,parent);if(!loader||e->ExceptionCheck())return false;
   jclass base=e->FindClass("java/lang/ClassLoader");if(!base)return false;
   jmethodID load=e->GetMethodID(base,"loadClass","(Ljava/lang/String;)Ljava/lang/Class;");if(!load)return false;
   jstring name=e->NewStringUTF("dev.a57.liveblur.ClockAodGuard");if(!name)return false;
   jclass guard=(jclass)e->CallObjectMethod(loader,load,name);if(!guard||e->ExceptionCheck())return false;
   jmethodID install=e->GetStaticMethodID(guard,"install","(Landroid/content/Context;)V");if(!install)return false;
   e->CallStaticVoidMethod(guard,install,app);return !e->ExceptionCheck();
  }();
  if(e->ExceptionCheck())e->ExceptionClear();
  e->PopLocalFrame(nullptr);
  if(!retry){__atomic_store_n(&finished,true,__ATOMIC_RELEASE);__android_log_print(ok?ANDROID_LOG_INFO:ANDROID_LOG_WARN,"A57ClockAod","Clock guard bootstrap %s",ok?"posted":"unavailable; native blur retained");}
 }else{if(e->ExceptionCheck())e->ExceptionClear();__atomic_store_n(&finished,true,__ATOMIC_RELEASE);}
 pthread_mutex_unlock(&mutex);
}
}
