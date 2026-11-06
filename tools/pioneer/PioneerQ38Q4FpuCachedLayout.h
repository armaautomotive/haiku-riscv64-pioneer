/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_CACHED_LAYOUT_H
#define PIONEER_Q38_Q4_FPU_CACHED_LAYOUT_H
#include "PioneerQ38Q4FpuCompactStreamLayout.h"
#include "PioneerQ38Bf16Wire.h"
// Returning matrix-service DATA. Canonical root partition/source/producer
// ownership remains EXTERNAL. Never aliases104's fixed source or native role.
struct PioneerQ38Q4FpuCachedLayout:PioneerQ38Q4FpuCompactStreamLayout {
 enum : unsigned {Magic=0x314d4351,Version=1,Output=0x84000,FrameWords=16,InputRoot=129,InputBase=0xa0000}; // QCM1
 struct Job {unsigned words[FrameWords];};
 struct Read {unsigned source,destination,bytes,route;};
 static unsigned GroupBytes(unsigned k,unsigned groups){return k==5120?groups*46080:groups*156672;}
 static unsigned Peer(unsigned worker){unsigned x=1;while(worker>=10){worker-=10;x++;}if(x>=8)x+=2;return x|((worker+2)<<6);}
 static bool Shape(const Job& j){
  const auto* w=j.words;
  if(w[0]!=Magic || w[1]!=Version || !Width(w[2]) || w[3]<0x04000000 || (w[3]&63)
   || !w[4] || w[4]>16 || (w[4]&1) || !Route(w[5]) || !w[6] || w[6]>=120 || w[7]!=Peer(w[6])
   || w[8]!=PioneerQ38Bf16Wire::Magic || w[9]!=InputBase || w[10]!=InputRoot
   || !w[11] || w[11]>=0x7fffffff || w[12]!=Output || w[13] || w[14] || w[15])return false;
  const unsigned bytes=GroupBytes(w[2],w[4]);
  return w[3]<=0xc4000000-bytes;
 }
private:
 friend class PioneerQ38Q4FpuCachedLoader;
 static bool SelectedPacket(const Job& j,unsigned pair,unsigned block,unsigned buffer,unsigned packet,Read& out){
  if(pair>=j.words[4]/2 || block>=j.words[2]/256 || buffer>=2 || packet>=Packets)return false;
  Read p={};const unsigned offset=buffer*BufferStride;
  if(packet<16){
   const unsigned face=packet%4,half=face/2;
   p={j.words[3]+GroupBytes(j.words[2],pair*2+half)+block*2304+256+(packet/4)*512+(face%2)*256,
      Raw+offset+packet*RawStride,256,j.words[5]};
  }else if(packet<18){
   p={j.words[3]+GroupBytes(j.words[2],pair*2+packet-16)+block*2304,Metadata+offset+(packet-16)*MetadataStride,256,j.words[5]};
  }else p={InputBase+block*512+(packet-18)*64,Input+offset+(packet-18)*InputStride,64,InputRoot};
  out=p;return true;
 }
public:
 static bool Packet(const Job& j,unsigned pair,unsigned block,unsigned buffer,unsigned packet,Read& out){
  return Shape(j) && SelectedPacket(j,pair,block,buffer,packet,out);
 }
 template<class IO> static bool Self(IO& io,unsigned worker){
  if(!worker || worker>=120)return false;
  const unsigned xy=Peer(worker),x=xy&63,y=xy>>6;
  const unsigned xi=x<6?0:x<12?1:2,yi=y<6?0:1;
  const unsigned xs=(x<6?x:x<12?x-6:x-12)*5,ys=(y<6?y:y-6)*5;
  const unsigned xa=0xffb20118+xi*4,ya=0xffb20130+yi*4;
  const unsigned node=io.Read(0xffb20044),shape=io.Read(0xffb20048),logical=io.Read(0xffb20148);
  const unsigned xt=io.Read(xa),yt=io.Read(ya),physical=((xt>>xs)&31)|(((yt>>ys)&31)<<6);
  return (logical&0xfff)==xy && (node&0xfff)==physical && ((node>>12)&127)==17 && ((node>>19)&127)==12
   && (shape&0xffffff00)==0x10000 && (io.Read(0xffb20100)&0x5000)==0x4000
   && !(io.Read(0xffb20150)&(1U<<x)) && !(io.Read(0xffb20154)&(1U<<y))
   && io.Read(0xffb20044)==node && io.Read(0xffb20048)==shape && io.Read(0xffb20148)==logical
   && io.Read(xa)==xt && io.Read(ya)==yt;
 }
};
#endif
