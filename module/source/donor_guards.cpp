#include <stdlib.h>
#include <android/log.h>
extern "C" void queueCount() asm("_ZN7android16BLASTBufferQueue20getQueuedBufferCountEv");
extern "C" void queueTime() asm("_ZN7android16BLASTBufferQueue35getTimeSpentPreviouslyWithoutBufferEv");
extern "C" void consumedTime() asm("_ZN7android16BLASTBufferQueue25getLastBufferConsumedTimeEv");
extern "C" void flingFlag() asm("_ZN7android16BLASTBufferQueue15setFlingStbFlagEb");
[[noreturn]] static void unexpected(){__android_log_print(ANDROID_LOG_FATAL,"NativeBlurDonor","Unexpected unrelated donor BLAST API");abort();}
extern "C" void queueCount(){unexpected();}
extern "C" void queueTime(){unexpected();}
extern "C" void consumedTime(){unexpected();}
extern "C" void flingFlag(){unexpected();}
