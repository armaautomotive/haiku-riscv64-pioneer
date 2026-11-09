/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_CPU_LAYER_SNAPSHOT_H
#define PIONEER_Q38_QGW8_CPU_LAYER_SNAPSHOT_H
#include "pioneer_q38_qgw8_cpu_hidden_snapshot.h"

// Fixed-token converted CPU DATA only, not device output or provenance.
// O_EXCL avoids overwriting existing data; an interrupted write is untrusted.
namespace Q38Qgw8CpuLayerSnapshot {
enum : unsigned {Layers=64,Width=5120,Words=Layers*Width};
inline bool Build(const std::vector<uint32_t>& converted,std::vector<uint32_t>& output){
 if(&converted==&output || converted.size()!=size_t(Layers+1)*Width)return false;
 for(size_t i=Width;i<converted.size();i++)if((converted[i]&0x7f800000U)==0x7f800000U)return false;
 std::vector<uint32_t> staged(converted.begin()+Width,converted.end());
 output=std::move(staged);return true;
}
inline bool SaveNew(const char* path,const std::vector<uint32_t>& words){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"CPU layer snapshot is little endian");
 if(!path || path[0]!='/' || words.size()!=Words)return false;
 for(uint32_t x:words)if((x&0x7f800000U)==0x7f800000U)return false;
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
