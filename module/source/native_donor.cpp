#include <sys/types.h>
#include "zygisk.hpp"
#include "donor_loader.hpp"
#include "expected_build.hpp"
#include <sys/system_properties.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
namespace {
donor::Fn exact=nullptr;
void *donorHandle=nullptr;
void forward(JNIEnv *e,jclass c,jlong t,jlong s,jobjectArray regions){
 static unsigned calls=0;
 if(__atomic_fetch_add(&calls,1,__ATOMIC_RELAXED)<3)
  __android_log_print(ANDROID_LOG_INFO,"NativeBlurDonor","Forwarding %d regions to unchanged Samsung Ex routine",regions?e->GetArrayLength(regions):0);
 if(exact)exact(e,c,t,s,regions);
}
bool matches(const char *s,const char *p){size_t n=strlen(p);return !strncmp(s,p,n)&&(s[n]==0||s[n]==':');}
bool target(const char *s){return !strcmp(s,"system:ui")||matches(s,"com.android.intentresolver")||matches(s,"com.android.systemui")||matches(s,"com.samsung.android.app.aodservice")||matches(s,"com.samsung.android.app.dressroom")||matches(s,"com.sec.android.app.launcher")||matches(s,"com.samsung.android.app.launcher");}
class NativeBlurModule:public zygisk::ModuleBase {
 zygisk::Api *api=nullptr;JNIEnv *env=nullptr;
 public:
 void onLoad(zygisk::Api *a,JNIEnv *e)override{api=a;env=e;}
 void preAppSpecialize(zygisk::AppSpecializeArgs *args)override{
  const char *name=args->nice_name?env->GetStringUTFChars(args->nice_name,nullptr):nullptr;
  bool selected=name&&target(name);char process[160]={};
  if(name){snprintf(process,sizeof(process),"%s",name);env->ReleaseStringUTFChars(args->nice_name,name);}
  if(!selected){api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);return;}
  char build[PROP_VALUE_MAX]={};__system_property_get("ro.build.fingerprint",build);
  if(strcmp(build,EXPECTED_BUILD)){__android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Firmware mismatch; no donor hook installed");return;}
  int fd=api->getModuleDir();
  if(fd<0){__android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Module directory unavailable");return;}
  char link[64],root[4096],path[4200];snprintf(link,sizeof(link),"/proc/self/fd/%d",fd);
  ssize_t n=readlink(link,root,sizeof(root)-1);close(fd);
  if(n<=0){__android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Module path unavailable");return;}
  root[n]=0;snprintf(path,sizeof(path),"%s/donor/libblur_loader.so",root);
  exact=donor::load(env,path,&donorHandle);
  if(!exact){if(env->ExceptionCheck())env->ExceptionClear();return;}
  JNINativeMethod method{(char*)"nativeSetBlurRegionsEx",(char*)"(JJ[Landroid/view/SemBlurRegionData;)V",(void*)forward};
  api->hookJniNativeMethods(env,"android/view/SurfaceControl",&method,1);
  if(method.fnPtr)__android_log_print(ANDROID_LOG_INFO,"NativeBlurDonor","Installed unchanged donor Ex in %s, pid %d",process,getpid());
  else{exact=nullptr;__android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","JNI hook failed in %s",process);}
 }
 void preServerSpecialize(zygisk::ServerSpecializeArgs*)override{api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);}
};
}
REGISTER_ZYGISK_MODULE(NativeBlurModule)
