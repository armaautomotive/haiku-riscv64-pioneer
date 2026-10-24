/* SPDX-License-Identifier: MIT */
// Actual diagnostic main,without a model file or any device/syscall access.
#define PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include "../PioneerQwen38PackedPlan.h"
static unsigned modelAttempts=0,deviceAttempts=0,ioctlAttempts=0,closeAttempts=0;
static FILE* UnavailableOriginalModel(const char* path,const char* mode){
 assert(!strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
 modelAttempts++;errno=ENOENT;return nullptr;
}
#define fopen UnavailableOriginalModel
#include "../PioneerQ38GdnOutputValidationSession.h"
#undef fopen
static int ForbiddenDeviceOpen(const char*,int){deviceAttempts++;assert(!"CLI must not open accelerator before authenticated source");return -1;}
static int ForbiddenDeviceClose(int){closeAttempts++;assert(!"CLI has no accelerator descriptor to close");return -1;}
static int ForbiddenDeviceIoctl(int,unsigned long,void*,size_t){ioctlAttempts++;assert(!"CLI must not issue device IO before authenticated source");return -1;}
#define open ForbiddenDeviceOpen
#define close ForbiddenDeviceClose
#define ioctl ForbiddenDeviceIoctl
#define main GdnOutputDiagnosticMain
#include "../pioneer_q38_gdn_output_validation.cpp"
#undef main
#undef ioctl
#undef close
#undef open
int main(){
 char name[]="debug55",valid[]="--validate-token0",other[]="--validate-token1",work[]="--work",path[]="arbitrary.gguf";
 char* missing[]={name,nullptr};char* good[]={name,valid,nullptr};char* excess[]={name,valid,path,nullptr};
 char* bad[]={name,other,nullptr};char* inference[]={name,work,nullptr};char* arbitrary[]={name,path,nullptr};
 assert(GdnOutputDiagnosticMain(0,nullptr)==2 && GdnOutputDiagnosticMain(1,missing)==2
  && GdnOutputDiagnosticMain(3,excess)==2 && GdnOutputDiagnosticMain(2,bad)==2
  && GdnOutputDiagnosticMain(2,inference)==2 && GdnOutputDiagnosticMain(2,arbitrary)==2);
 assert(!modelAttempts && !deviceAttempts && !ioctlAttempts && !closeAttempts);
 assert(GdnOutputDiagnosticMain(2,good)==1 && modelAttempts==1 && !deviceAttempts && !ioctlAttempts && !closeAttempts);
 puts("PASS actual DEBUG55 main/6 invalid CLI shapes BEFORE model IO/fixed token0 model-path-only failure BEFORE accelerator open/ioctl/close;NO device/model arithmetic/TPS");
}
