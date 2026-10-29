/* SPDX-License-Identifier: MIT */
// Host-only DATA serialization. A file/complete marker is never execution,
// authentication,ownership,STOP or cleanup authority.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error State snapshot serializer is HOST DATA only,never kernel,image,native or Work
#endif
#ifndef PIONEER_Q38_STATE_SNAPSHOT_DATA_H
#define PIONEER_Q38_STATE_SNAPSHOT_DATA_H
#include <array>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
namespace Q38StateSnapshotData {
constexpr uint32_t CpuMagic=0x31534351,NativeMagic=0x31534e51,End=0x444e4553;
inline unsigned Count(unsigned completed,unsigned layer,unsigned kind){
 if((completed!=2 && completed!=3) || layer>=64 || kind>3 || (completed==2 && kind!=0)
  || (layer%4==3?kind<2:kind>=2))return 0;
 return kind==0?786432:kind==1?30720:completed*512;
}
inline std::string Name(unsigned completed,unsigned layer,unsigned kind){
 return "c"+std::to_string(completed)+"-l"+std::to_string(layer)+"-k"+std::to_string(kind)+".bin";
}
inline bool Finite(unsigned kind,const uint32_t* words,unsigned count){
 if(!words || kind>3)return false;
 for(unsigned i=0;i<count;i++){
  if(kind<2){if((words[i]&0x7f800000)==0x7f800000)return false;}
  else if((words[i]&0x7c00)==0x7c00 || (words[i]&0x7c000000)==0x7c000000)return false;
 }return true;
}
inline bool Words(FILE* file,const uint32_t* words,unsigned count){
 std::array<uint8_t,4096> bytes;
 while(count){const unsigned n=count<1024?count:1024;
  for(unsigned i=0;i<n;i++)for(unsigned b=0;b<4;b++)bytes[i*4+b]=uint8_t(words[i]>>(b*8));
  if(fwrite(bytes.data(),1,n*4,file)!=n*4)return false;words+=n;count-=n;
 }return true;
}
inline bool Write(const char* directory,const std::string& name,const uint32_t* header,const uint32_t* words,unsigned count){
 if(!directory || directory[0]!='/' || name.empty() || name.find('/')!=std::string::npos)return false;
 struct stat named={},opened={};if(lstat(directory,&named) || !S_ISDIR(named.st_mode))return false;
 int dir=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(dir<0)return false;
 if(fstat(dir,&opened) || opened.st_dev!=named.st_dev || opened.st_ino!=named.st_ino){close(dir);return false;}
 int fd=openat(dir,name.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0){close(dir);return false;}
 FILE* file=fdopen(fd,"wb");if(!file){close(fd);close(dir);return false;}
 bool okay=Words(file,header,16) && Words(file,words,count) && Words(file,&End,1);
 okay=!fflush(file) && !ferror(file) && okay;const int closed=fclose(file),dirClosed=close(dir);
 return okay && !closed && !dirClosed;
}
inline bool Save(const char* directory,bool native,unsigned completed,unsigned layer,unsigned kind,const uint32_t* words,unsigned count){
 if(!count || count!=Count(completed,layer,kind) || !Finite(kind,words,count))return false;
 const uint32_t header[16]={native?NativeMagic:CpuMagic,1,completed,layer,kind,count};
 return Write(directory,Name(completed,layer,kind),header,words,count);
}
inline bool Finish(const char* directory,bool native,unsigned records){
 if(records!=176)return false;
 const uint32_t header[16]={native?NativeMagic:CpuMagic,1,176,3,64};
 return Write(directory,"state-complete.bin",header,nullptr,0);
}
}
#endif
