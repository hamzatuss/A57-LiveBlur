#pragma once
#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <stdint.h>
namespace donor {
using Fn=void(*)(JNIEnv*,jclass,jlong,jlong,jobjectArray);
using Registrar=int(*)(JNIEnv*);
struct Loaded { void *handle=nullptr; Registrar registrar=nullptr; Fn entry=nullptr; };
static unsigned suppressed=0;
static jint noRegister(JNIEnv*,jclass,const JNINativeMethod*,jint n){suppressed+=(unsigned)n;return JNI_OK;}
// Map and validate while the Zygisk child still has access to module files.
// JNI caches are initialized later, only if this app calls the blur API.
inline bool open(const char *path,Loaded *result){
 void *h=dlopen(path,RTLD_NOW|RTLD_LOCAL);
 if(!h){__android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Load failed: %s",dlerror());return false;}
 auto reg=(Registrar)dlsym(h,"_ZN7android36register_android_view_SurfaceControlEP7_JNIEnv");Dl_info info{};
 if(!reg||!dladdr((void*)reg,&info)||(uintptr_t)reg-(uintptr_t)info.dli_fbase!=0x1724d0){
  __android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Donor identity mismatch");dlclose(h);return false;
 }
 auto fn=(Fn)((uintptr_t)info.dli_fbase+0x2276cc);
 const unsigned char expected[]={0x3f,0x23,0x03,0xd5,0xff,0x43,0x07,0xd1};auto bytes=(const unsigned char*)fn;
 for(unsigned i=0;i<sizeof(expected);i++)if(bytes[i]!=expected[i]){dlclose(h);return false;}
 result->handle=h;result->registrar=reg;result->entry=fn;return true;
}
// The caller serializes initialization within each specialized app process.
inline Fn initialize(JNIEnv *env,const Loaded &loaded){
 const JNINativeInterface *saved=env->functions;JNINativeInterface local=*saved;local.RegisterNatives=noRegister;
 env->functions=&local;int result=loaded.registrar(env);env->functions=saved;
 if(result!=JNI_OK||env->ExceptionCheck()){
  if(env->ExceptionCheck())env->ExceptionClear();
  __android_log_print(ANDROID_LOG_ERROR,"NativeBlurDonor","Cache initialization failed");return nullptr;
 }
 __android_log_print(ANDROID_LOG_INFO,"NativeBlurDonor","Initialized original donor JNI caches on first blur use; suppressed %u unrelated registrations",suppressed);
 return loaded.entry;
}
}
