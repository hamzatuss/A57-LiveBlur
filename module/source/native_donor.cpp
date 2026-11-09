#include <sys/types.h>
#include "zygisk.hpp"
#include "donor_loader.hpp"
#include "expected_build.hpp"
#include <sys/system_properties.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
namespace {
donor::Loaded loaded;
donor::Fn exact=nullptr,original=nullptr;
pthread_mutex_t initMutex=PTHREAD_MUTEX_INITIALIZER;
bool initDone=false;
void forward(JNIEnv *e,jclass c,jlong t,jlong s,jobjectArray regions){
 if(!__atomic_load_n(&initDone,__ATOMIC_ACQUIRE)){
  pthread_mutex_lock(&initMutex);
  if(!__atomic_load_n(&initDone,__ATOMIC_RELAXED)){
   exact=donor::initialize(e,loaded);
   __atomic_store_n(&initDone,true,__ATOMIC_RELEASE);
  }
  pthread_mutex_unlock(&initMutex);
 }
 static unsigned calls=0;
 if(__atomic_fetch_add(&calls,1,__ATOMIC_RELAXED)<3)
  __android_log_print(ANDROID_LOG_INFO,"NativeBlurDonor","Forwarding %d regions to Samsung Ex routine",regions?e->GetArrayLength(regions):0);
 if(exact)exact(e,c,t,s,regions);else if(original)original(e,c,t,s,regions);
}
bool appProcess(const char *name,bool childZygote){
 if(!name||!name[0]||childZygote)return false;
 return strcmp(name,"system_server")&&strcmp(name,"system")&&strcmp(name,"zygote")&&strcmp(name,"zygote64")&&strcmp(name,"webview_zygote");
}
class NativeBlurModule:public zygisk::ModuleBase {
 zygisk::Api *api=nullptr;JNIEnv *env=nullptr;
 public:
 void onLoad(zygisk::Api *a,JNIEnv *e)override{api=a;env=e;}
 void preAppSpecialize(zygisk::AppSpecializeArgs *args)override{
  const char *name=args->nice_name?env->GetStringUTFChars(args->nice_name,nullptr):nullptr;
  bool child=args->is_child_zygote&&*args->is_child_zygote;
  bool selected=appProcess(name,child);char process[160]={};
  if(name){snprintf(process,sizeof(process),"%s",name);env->ReleaseStringUTFChars(args->nice_name,name);}
  if(!selected){api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;}
  char build[PROP_VALUE_MAX]={};__system_property_get("ro.build.fingerprint",build);
  if(strcmp(build,EXPECTED_BUILD)){
   __android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Firmware mismatch; no donor hook installed");api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;
  }
  int fd=api->getModuleDir();
  if(fd<0){__android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Module directory unavailable");api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;}
  char link[64],root[4096],path[4200];snprintf(link,sizeof(link),"/proc/self/fd/%d",fd);
  ssize_t n=readlink(link,root,sizeof(root)-1);close(fd);
  if(n<=0){__android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Module path unavailable");api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;}
  root[n]=0;snprintf(path,sizeof(path),"%s/donor/libblur_loader.so",root);
  if(!donor::open(path,&loaded)){api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;}
  JNINativeMethod method{(char*)"nativeSetBlurRegionsEx",(char*)"(JJ[Landroid/view/SemBlurRegionData;)V",(void*)forward};
  api->hookJniNativeMethods(env,"android/view/SurfaceControl",&method,1);
  original=(donor::Fn)method.fnPtr;
  if(original)__android_log_print(ANDROID_LOG_INFO,"NativeBlurDonor","Installed Samsung Ex hook in %s, pid %d; JNI caches deferred until use",process,getpid());
  else __android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","JNI hook failed in %s",process);
 }
 void preServerSpecialize(zygisk::ServerSpecializeArgs*)override{api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);}
};
}
REGISTER_ZYGISK_MODULE(NativeBlurModule)
