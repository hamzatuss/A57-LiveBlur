#include <sys/types.h>
#include "zygisk.hpp"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct { jclass rectClass; } schema;
static zygisk::internal::module_abi *moduleAbi=nullptr;
static unsigned hooked=0;
static bool fakeRegister(zygisk::internal::api_table*,zygisk::internal::module_abi *m){moduleAbi=m;return true;}
static void fakeHook(JNIEnv *e,const char *name,JNINativeMethod *methods,int n){
 assert(n==1&&!strcmp(name,"android/view/SurfaceControl")&&!strcmp(methods[0].name,"nativeSetBlurRegionsEx"));
 auto cls=e->FindClass(name);JNINativeMethod patch=methods[0];assert(e->RegisterNatives(cls,&patch,1)==JNI_OK);
 Dl_info info{};assert(dladdr(dlsym(RTLD_DEFAULT,"_ZN7android36register_android_view_SurfaceControlEP7_JNIEnv"),&info));
 methods[0].fnPtr=(char*)info.dli_fbase+0x2167e8;++hooked;
}
static const char *moduleRoot(){const char *p=getenv("NATIVEBLUR_MODULE_DIR");assert(p&&p[0]=='/');return p;}
static int fakeDir(void*){return open(moduleRoot(),O_RDONLY|O_DIRECTORY);}
static bool unloaded=false;
static void fakeOption(void*,zygisk::Option option){assert(option==zygisk::Option::DLCLOSE_MODULE_LIBRARY);unloaded=true;}
static void installModuleFixture(JNIEnv *e){
 char modulePath[4096];snprintf(modulePath,sizeof(modulePath),"%s/zygisk/arm64-v8a.so",moduleRoot());
 void *h=dlopen(modulePath,RTLD_NOW|RTLD_LOCAL);if(!h){puts(dlerror());assert(false);}
 using Entry=void(*)(zygisk::internal::api_table*,JNIEnv*);auto entry=(Entry)dlsym(h,"zygisk_module_entry");assert(entry);
 static zygisk::internal::api_table table{};table.registerModule=fakeRegister;table.hookJniNativeMethods=fakeHook;table.getModuleDir=fakeDir;table.setOption=fakeOption;
 entry(&table,e);assert(moduleAbi&&moduleAbi->api_version==4);
 jint uid=1000,gid=1000,flags=0,mount=0;jintArray gids=nullptr;jobjectArray limits=nullptr;
 jstring se=nullptr,name=e->NewStringUTF(getenv("NATIVEBLUR_TEST_TARGET")?getenv("NATIVEBLUR_TEST_TARGET"):"system:ui"),isa=e->NewStringUTF("arm64"),dir=nullptr;
 zygisk::AppSpecializeArgs args{uid,gid,gids,flags,limits,mount,se,name,isa,dir,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
 if(getenv("NATIVEBLUR_TEST_SERVER")&&!strcmp(getenv("NATIVEBLUR_TEST_SERVER"),"1"))moduleAbi->preServerSpecialize(moduleAbi->impl,nullptr);else moduleAbi->preAppSpecialize(moduleAbi->impl,&args);
 bool expect=!(getenv("NATIVEBLUR_EXPECT_NO_HOOK")&&!strcmp(getenv("NATIVEBLUR_EXPECT_NO_HOOK"),"1"));assert(hooked==(expect?1u:0u)&&unloaded!=expect&&!e->ExceptionCheck());
 puts("Production Zygisk module registered only the extended-blur method in the isolated runtime.");
}


static jmethodID method(JNIEnv *e,jclass c,const char *name,const char *sig){
 auto m=e->GetMethodID(c,name,sig);if(!m){e->ExceptionDescribe();fprintf(stderr,"Missing %s %s\n",name,sig);abort();}return m;
}
static unsigned capturePixel(JNIEnv *e,jobject root){
 auto bc=e->FindClass("android/window/ScreenCaptureInternal$LayerCaptureArgs$Builder");
 auto builder=e->NewObject(bc,method(e,bc,"<init>","(Landroid/view/SurfaceControl;)V"),root);
 auto rect=e->NewObject(schema.rectClass,method(e,schema.rectClass,"<init>","(IIII)V"),0,0,64,64);
 e->CallObjectMethod(builder,method(e,bc,"setSourceCrop","(Landroid/graphics/Rect;)Landroid/window/ScreenCaptureInternal$CaptureArgs$Builder;"),rect);
 e->CallObjectMethod(builder,method(e,bc,"setChildrenOnly","(Z)Landroid/window/ScreenCaptureInternal$LayerCaptureArgs$Builder;"),JNI_TRUE);
 auto args=e->CallObjectMethod(builder,method(e,bc,"build","()Landroid/window/ScreenCaptureInternal$LayerCaptureArgs;"));
 auto sc=e->FindClass("android/window/ScreenCaptureInternal");
 auto capture=e->CallStaticObjectMethod(sc,e->GetStaticMethodID(sc,"captureLayers","(Landroid/window/ScreenCaptureInternal$LayerCaptureArgs;)Landroid/window/ScreenCaptureInternal$ScreenshotHardwareBuffer;"),args);
 if(!capture){e->ExceptionDescribe();fputs("Offscreen capture failed\n",stderr);abort();}
 auto hc=e->GetObjectClass(capture);auto bitmap=e->CallObjectMethod(capture,method(e,hc,"asBitmap","()Landroid/graphics/Bitmap;"));
 auto bitmapClass=e->FindClass("android/graphics/Bitmap"),configClass=e->FindClass("android/graphics/Bitmap$Config");
 auto config=e->GetStaticObjectField(configClass,e->GetStaticFieldID(configClass,"ARGB_8888","Landroid/graphics/Bitmap$Config;"));
 auto cpu=e->CallObjectMethod(bitmap,method(e,bitmapClass,"copy","(Landroid/graphics/Bitmap$Config;Z)Landroid/graphics/Bitmap;"),config,JNI_FALSE);
 assert(cpu&&!e->ExceptionCheck());
 unsigned color=(unsigned)e->CallIntMethod(cpu,method(e,bitmapClass,"getPixel","(II)I"),32,32);
 e->CallVoidMethod(cpu,method(e,bitmapClass,"recycle","()V"));e->CallVoidMethod(bitmap,method(e,bitmapClass,"recycle","()V"));
 return color;
}
static void offscreenCurveTest(JNIEnv *e){
 auto builderClass=e->FindClass("android/view/SurfaceControl$Builder"),controlClass=e->FindClass("android/view/SurfaceControl"),tc=e->FindClass("android/view/SurfaceControl$Transaction");
 auto newBuilder=method(e,builderClass,"<init>","()V"),name=method(e,builderClass,"setName","(Ljava/lang/String;)Landroid/view/SurfaceControl$Builder;"),build=method(e,builderClass,"build","()Landroid/view/SurfaceControl;");
 auto parentBuilder=e->NewObject(builderClass,newBuilder);
 e->CallObjectMethod(parentBuilder,name,e->NewStringUTF("NativeBlur offscreen color-curve test"));
 e->CallObjectMethod(parentBuilder,method(e,builderClass,"setContainerLayer","()Landroid/view/SurfaceControl$Builder;"));
 auto root=e->CallObjectMethod(parentBuilder,build);assert(root&&!e->ExceptionCheck());
 auto bufferBuilder=e->NewObject(builderClass,newBuilder);e->CallObjectMethod(bufferBuilder,name,e->NewStringUTF("NativeBlur black background"));
 e->CallObjectMethod(bufferBuilder,method(e,builderClass,"setParent","(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Builder;"),root);
 e->CallObjectMethod(bufferBuilder,method(e,builderClass,"setBufferSize","(II)Landroid/view/SurfaceControl$Builder;"),64,64);
 auto buffer=e->CallObjectMethod(bufferBuilder,build);
 auto effectBuilder=e->NewObject(builderClass,newBuilder);e->CallObjectMethod(effectBuilder,name,e->NewStringUTF("NativeBlur curve effect"));
 e->CallObjectMethod(effectBuilder,method(e,builderClass,"setParent","(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Builder;"),root);
 e->CallObjectMethod(effectBuilder,method(e,builderClass,"setEffectLayer","()Landroid/view/SurfaceControl$Builder;"));
 auto effect=e->CallObjectMethod(effectBuilder,build);
 auto surfaceClass=e->FindClass("android/view/Surface");auto surface=e->NewObject(surfaceClass,method(e,surfaceClass,"<init>","(Landroid/view/SurfaceControl;)V"),buffer);
 auto canvas=e->CallObjectMethod(surface,method(e,surfaceClass,"lockCanvas","(Landroid/graphics/Rect;)Landroid/graphics/Canvas;"),nullptr);
 auto canvasClass=e->FindClass("android/graphics/Canvas");e->CallVoidMethod(canvas,method(e,canvasClass,"drawColor","(I)V"),(jint)0xff000000);
 auto paintClass=e->FindClass("android/graphics/Paint");auto paint=e->NewObject(paintClass,method(e,paintClass,"<init>","()V"));e->CallVoidMethod(paint,method(e,paintClass,"setColor","(I)V"),(jint)0xffffffff);e->CallVoidMethod(canvas,method(e,canvasClass,"drawRect","(FFFFLandroid/graphics/Paint;)V"),0.0,0.0,32.0,64.0,paint);
 e->CallVoidMethod(surface,method(e,surfaceClass,"unlockCanvasAndPost","(Landroid/graphics/Canvas;)V"),canvas);
 auto transaction=e->NewObject(tc,method(e,tc,"<init>","()V"));
 e->CallObjectMethod(transaction,method(e,tc,"setLayerStack","(Landroid/view/SurfaceControl;I)Landroid/view/SurfaceControl$Transaction;"),root,0x7fff1234);
 auto show=method(e,tc,"show","(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
 e->CallObjectMethod(transaction,show,root);e->CallObjectMethod(transaction,show,buffer);e->CallObjectMethod(transaction,show,effect);
 auto layer=method(e,tc,"setLayer","(Landroid/view/SurfaceControl;I)Landroid/view/SurfaceControl$Transaction;");
 e->CallObjectMethod(transaction,layer,buffer,0);e->CallObjectMethod(transaction,layer,effect,1);
 auto apply=method(e,tc,"apply","()V");e->CallVoidMethod(transaction,apply);assert(!e->ExceptionCheck());usleep(200000);
 unsigned before=capturePixel(e,root);
 auto regionClass=e->FindClass("android/view/SemBlurRegionData");auto r=e->NewObject(regionClass,method(e,regionClass,"<init>","()V"));
 auto rect=e->NewObject(schema.rectClass,method(e,schema.rectClass,"<init>","(IIII)V"),0,0,64,64);
 e->SetIntField(r,e->GetFieldID(regionClass,"mBlurRadius","I"),40);e->SetFloatField(r,e->GetFieldID(regionClass,"mAlpha","F"),1);e->SetObjectField(r,e->GetFieldID(regionClass,"mRect","Landroid/graphics/Rect;"),rect);
 auto curveClass=e->FindClass("android/view/SemBlurInfo$ColorCurve");auto curve=e->AllocObject(curveClass);
 const char *names[]={"mMinX","mMinY","mMaxX","mMaxY","mCurveBias","mSaturation"};float values[]={15,169.6f,235,246.1f,30,0.55f};
 for(int i=0;i<6;i++)e->SetFloatField(curve,e->GetFieldID(curveClass,names[i],"F"),values[i]);
 e->SetObjectField(r,e->GetFieldID(regionClass,"mColorCurve","Landroid/view/SemBlurInfo$ColorCurve;"),curve);
 auto regions=e->NewObjectArray(1,regionClass,r);
 auto native=e->GetStaticMethodID(controlClass,"nativeSetBlurRegionsEx","(JJ[Landroid/view/SemBlurRegionData;)V");
 auto tn=e->GetFieldID(tc,"mNativeObject","J"),sn=e->GetFieldID(controlClass,"mNativeObject","J");
 auto curveField=e->GetFieldID(regionClass,"mColorCurve","Landroid/view/SemBlurInfo$ColorCurve;");e->SetObjectField(r,curveField,nullptr);
 e->CallStaticVoidMethod(controlClass,native,e->GetLongField(transaction,tn),e->GetLongField(effect,sn),regions);
 e->CallVoidMethod(transaction,apply);assert(!e->ExceptionCheck());usleep(200000);unsigned plain=capturePixel(e,root);
 e->SetObjectField(r,curveField,curve);
 e->CallStaticVoidMethod(controlClass,native,e->GetLongField(transaction,tn),e->GetLongField(effect,sn),regions);
 e->CallVoidMethod(transaction,apply);assert(!e->ExceptionCheck());usleep(200000);
 unsigned after=capturePixel(e,root);
 assert(before!=plain && after!=plain);
 printf("Offscreen checkerboard: baseline=0x%08x, plain blur=0x%08x, Samsung curve=0x%08x\n",before,plain,after);fflush(stdout);
 e->CallVoidMethod(surface,method(e,surfaceClass,"release","()V"));
 e->CallVoidMethod(transaction,method(e,tc,"close","()V"));
 auto release=method(e,controlClass,"release","()V");e->CallVoidMethod(effect,release);e->CallVoidMethod(buffer,release);e->CallVoidMethod(root,release);
}

extern "C" __attribute__((visibility("default"))) void Java_BlurBridgeTest_run(JNIEnv *e,jclass){
 installModuleFixture(e);
 if(unloaded){puts("Excluded process received no JNI hook.");return;}
 schema.rectClass=e->FindClass("android/graphics/Rect");assert(schema.rectClass&&!e->ExceptionCheck());
 offscreenCurveTest(e);
}
