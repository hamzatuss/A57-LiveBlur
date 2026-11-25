#pragma once
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
namespace scope {
// Package entries also cover colon-named subprocesses. No wildcard matching.
inline bool matches(const char *name,const char *entry){
 size_t n=strlen(entry);
 return n&&strncmp(name,entry,n)==0&&(name[n]==0||name[n]==':');
}
inline bool excluded(int moduleDir,const char *name){
 int fd=openat(moduleDir,"excluded-apps.txt",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
 if(fd<0)return false;
 FILE *file=fdopen(fd,"r");if(!file){close(fd);return false;}
 char line[512];bool found=false;
 while(fgets(line,sizeof(line),file)){
  // Reject an overlong line rather than accidentally matching a prefix.
  if(!strchr(line,'\n')&&!feof(file)){int c;while((c=fgetc(file))!=EOF&&c!='\n'){}continue;}
  char *comment=strchr(line,'#');if(comment)*comment=0;
  char *start=line;while(*start==' '||*start=='\t')++start;
  size_t n=strlen(start);while(n&&(start[n-1]==' '||start[n-1]=='\t'||start[n-1]=='\r'||start[n-1]=='\n'))start[--n]=0;
  if(matches(name,start)){found=true;break;}
 }
 fclose(file);return found;
}
}
