/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_CPU_HIDDEN_SNAPSHOT_H
#define PIONEER_Q38_QGW8_CPU_HIDDEN_SNAPSHOT_H
#if !defined(PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC) || PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error QGW8 CPU hidden snapshot is isolated reference data only
#endif
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

// Fixed little-endian CPU reference data, NEVER an accelerator snapshot or
// device input. A failed exclusive write leaves an untrusted partial file.
namespace Q38Qgw8CpuHiddenSnapshot {
enum : uint32_t {Magic=0x38484351,Version=1,Input=248319,Layers=64,Width=5120,
 Vocabulary=248320,Header=16,Arrays=2,Footer=0x454e4448};
inline bool Build(const std::vector<uint32_t>& original,const std::vector<uint32_t>& converted,
 unsigned originalTop,unsigned convertedTop,std::vector<uint32_t>& output){
 if(&output==&original || &output==&converted || original.size()!=size_t(Layers+1)*Width || converted.size()!=original.size()
  || originalTop>=Vocabulary || convertedTop>=Vocabulary)return false;
 std::vector<uint32_t> staged(Header+Arrays*Width+1,0);
 const uint32_t h[]={Magic,Version,Input,Layers,Width,Arrays,originalTop,convertedTop};
 for(unsigned i=0;i<8;i++)staged[i]=h[i];
 for(unsigned i=0;i<Width;i++){
  const uint32_t a=original[size_t(Layers)*Width+i],b=converted[size_t(Layers)*Width+i];
  if((a&0x7f800000U)==0x7f800000U || (b&0x7f800000U)==0x7f800000U)return false;
  staged[Header+i]=a;staged[Header+Width+i]=b;
 }
 staged.back()=Footer;output=std::move(staged);return true;
}
inline bool Shape(const std::vector<uint32_t>& words){
 if(words.size()!=Header+Arrays*Width+1 || words[0]!=Magic || words[1]!=Version || words[2]!=Input
  || words[3]!=Layers || words[4]!=Width || words[5]!=Arrays || words[6]>=Vocabulary || words[7]>=Vocabulary
  || words.back()!=Footer)return false;
 for(unsigned i=8;i<Header;i++)if(words[i])return false;
 for(unsigned i=Header;i<Header+Arrays*Width;i++)if((words[i]&0x7f800000U)==0x7f800000U)return false;
 return true;
}
inline bool SaveNew(const char* path,const std::vector<uint32_t>& words){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"CPU snapshot format is little endian");
 if(!path || path[0]!='/' || !Shape(words))return false;
 const int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return false;
 const auto* data=reinterpret_cast<const uint8_t*>(words.data());size_t left=words.size()*4;bool okay=true;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1 || info.st_uid!=geteuid()
  || (info.st_mode&077)!=0)okay=false;
 while(okay && left){const ssize_t written=write(fd,data,left);
  if(written<0 && errno==EINTR)continue;
  if(written<=0){okay=false;break;}data+=written;left-=size_t(written);
 }
 if(okay && fsync(fd))okay=false;
 if(close(fd))okay=false;
 return okay && !left;
}
}
#endif
