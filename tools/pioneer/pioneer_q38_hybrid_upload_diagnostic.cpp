/* SPDX-License-Identifier: MIT */
// Isolated upload-and-release diagnostic: NO token/model execution.
#if !defined(PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC) || PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1
#error Hybrid upload diagnostic needs both explicit isolated roles
#endif
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "PioneerQ38HybridVerifiedModel.h"
#include "PioneerQ38HybridUploadSession.h"

struct Transport {
 int Open(){return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);}
 bool Call(int fd,unsigned op,void* data,size_t size){return ioctl(fd,op,data,size)==0;}
 int Close(int fd){return close(fd);}
};
static uint64_t Now(){
 timespec t={};if(clock_gettime(CLOCK_MONOTONIC,&t) || t.tv_sec<0 || t.tv_nsec<0 || t.tv_nsec>=1000000000)return 0;
 return uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);
}
int main(int argc,char** argv){
 if(argc!=4 || strcmp(argv[1],"--upload-verified-diagnostic") || argv[2][0]!='/' || argv[3][0]!='/'){
  fputs("Use --upload-verified-diagnostic ABSOLUTE_BFP4_CACHE ABSOLUTE_BFP8_CACHE; upload and release only.\n",stderr);return 2;
 }
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("HYBRID_UPLOAD_DIAGNOSTIC source authentication and cache verification; NO DEVICE OPEN YET");
 auto model=std::make_unique<PioneerQ38HybridVerifiedModel>();std::string error;
 if(!model->Prepare(argv[2],argv[3],error,[](unsigned done,unsigned total){
  if(done%21==0 || done==total)printf("HYBRID_UPLOAD_PREFLIGHT %u/%u\n",done,total);
 }) || !model->Ready()){
  fprintf(stderr,"HYBRID_UPLOAD_PREFLIGHT_REFUSED %s; NO DEVICE OPEN\n",error.c_str());return 1;
 }
 auto* pages=model->PreparedPages();const auto* canonical=model->Canonical();
 if(!pages || !canonical)return 1;
 Transport io;PioneerQ38HybridUploadSession<PioneerQ38HybridVerifiedModel::Pages,Transport> session(*pages,io);
 const uint64_t start=Now();if(!start)return 1;
 uint64_t next=UINT64_C(256)*1024*1024;
 if(!session.Run(*canonical,error,[&](uint64_t done,uint64_t total){
  if(done>=next || done==total){printf("HYBRID_UPLOAD_PROGRESS %llu/%llu bytes; NO MODEL EXECUTION\n",
   (unsigned long long)done,(unsigned long long)total);while(next<=done)next+=UINT64_C(256)*1024*1024;}
 })){
  fprintf(stderr,"HYBRID_UPLOAD_REFUSED %s; CLOSE ONLY, NO END RETRY OR RESET\n",error.c_str());return 1;
 }
 const uint64_t end=Now();if(!end || end<start || !session.Complete())return 1;
 printf("PASS_HYBRID_UPLOAD_RELEASE fragments%u spans%u bytes%llu seconds%.3f; "
  "SPANS_AND_BANKS_ENDED WEIGHTS_RELEASED NO_MODEL_EXECUTION_OR_TOKEN_RATE\n",
  session.Upload().UploadedParts(),session.Upload().UploadedSpans(),
  (unsigned long long)session.Upload().UploadedBytes(),double(end-start)*1e-9);
 return fflush(stdout)?1:0;
}
