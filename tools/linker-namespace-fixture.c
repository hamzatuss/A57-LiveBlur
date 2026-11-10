#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void) {
 char exe[4096]={0}, resolved[4096];
 readlink("/proc/self/exe",exe,sizeof(exe)-1);
 void* (*getns)(const char*)=dlsym(RTLD_DEFAULT,"android_get_exported_namespace");
 printf("exe=%s\nrealpath=%s\nsphal=%d default=%d\n",exe,realpath("/proc/self/exe",resolved)?resolved:"FAILED",getns&&getns("sphal")!=NULL,getns&&getns("default")!=NULL);
 return getns&&getns("sphal")?0:2;
}
