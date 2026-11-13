/* SPDX-License-Identifier: MIT */
// Offline validation fixture: authenticate the whole original model before
// exporting only layer-0 FP32 recurrent parameters. No device or Work path.
#define PIONEER_Q38_CPU_VALIDATION 1
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
static const char* modelPath=nullptr;
static FILE* LocalModelOpen(const char* path,const char* mode){
 assert(modelPath && !std::strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !std::strcmp(mode,"rb"));
 return std::fopen(modelPath,mode);
}
static int LocalHashExec(const char* path,char* const* argv){
 assert(!std::strcmp(path,"/bin/sha256sum") && !std::strcmp(argv[0],"sha256sum") && !argv[1]);
 char a[]="shasum",b[]="-a",c[]="256";char* args[]={a,b,c,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen LocalModelOpen
#define execv LocalHashExec
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv

int main(int argc,char** argv){
 if(argc!=3 || argv[1][0]!='/' || argv[2][0]!='/')return 2;
 modelPath=argv[1];PioneerQ38PublicModelSource source;assert(source.Open() && source.Ready());
 const char* names[]={"blk.0.ssm_conv1d.weight","blk.0.ssm_a","blk.0.ssm_dt.bias","blk.0.ssm_norm.weight"};
 const unsigned sizes[]={163840,192,192,512},inputs[]={4,48,48,128},outputs[]={10240,1,1,1};
 std::vector<uint8_t> values;values.reserve(164736);
 for(unsigned i=0;i<4;i++){
  const auto* t=source.Model().Find(names[i]);
  assert(t && !t->type && t->inputs==inputs[i] && t->outputs==outputs[i] && t->bytes==sizes[i]);
  unsigned index=unsigned(t-source.Model().Tensors().data());const size_t at=values.size();values.resize(at+sizes[i]);
  assert(source.ReadReferencePacked(index,0,sizes[i],values.data()+at,sizes[i]) && source.Ready());
 }
 int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);assert(fd>=0);
 size_t at=0;while(at<values.size()){ssize_t n=write(fd,values.data()+at,values.size()-at);assert(n>0);at+=size_t(n);}
 assert(!fsync(fd) && !close(fd) && source.Ready());
 std::printf("PASS authenticated original layer-0 Conv/A/dt/norm export bytes%zu; NO DEVICE OR SPEED\n",values.size());
}
