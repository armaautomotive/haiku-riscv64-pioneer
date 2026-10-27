/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_COHORT_LAYOUT_H
#define PIONEER_Q38_MIXED_RECURRENT_COHORT_LAYOUT_H
#include "PioneerQ38MixedWorkerLayout.h"
#include "PioneerQ38MixedRecurrentDispatch.h"
namespace PioneerQ38MixedRecurrentCohortLayout {
// Phase-only tables must NOT overwrite the shared matrix controller's
// live peer counters/nonces/boot/job records. No native mapping grant.
enum {Magic=0x31435251,Fault=0x46435251,Code=0x40000,ConvCode=0x64000,
 Header=0x74600,Boot=0x75300,Doorbell=0x75400,Status=0x75440,Summary=0x75500,Scratch=0x78000,
 Conv=0xc8000,ConvShadow=0x128000,ConvWords=120*384,
 Gdn=0x100000,GdnShadow=0x155000,GdnWords=96*192,
 Counters=0x122000,Nonces=0x123000,PeerBoots=0x170000,PeerJobs=0x178000,End=0x17f700};
struct PrivateSeal {uint32_t boot[64],frame[64];};
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic || b[1]!=1 || b[2]!=64 || b[3]!=129 || b[4]!=b[16] || !b[4]
  || b[5]!=119 || b[6]!=8192 || b[7]!=64)return false;
 for(unsigned i=8;i<16;i+=2)if(!(b[i]|b[i+1]))return false;
 for(unsigned i=32;i<64;i++)if(b[i])return false;
 return true;
}
static inline bool Shape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=64 || d[3]>=64 || d[3]%4==3 || d[4]>=8192
  || !d[5] || d[5]>=0x7ffffffd || d[6]!=d[5]+1 || d[7]>=7 || (d[8]&4095)
  || d[8]<0x04000000 || d[8]>0xc0000000 || d[9]!=PioneerQ38ProjectionRoute(d[7])
  || d[10]!=129 || d[11]!=119 || d[12]!=96 || d[13]!=640 || d[14]>1 || d[15]>=248320 || !d[16]
  || d[40]!=Conv || d[41]!=ConvShadow || d[42]!=Gdn || d[43]!=GdnShadow
  || d[44]!=ConvWords || d[45]!=GdnWords || d[46]!=PeerBoots || d[47]!=PeerJobs
  || !d[48] || d[48]>=0x7fffffff || d[49]!=Nonces || d[50]!=Counters || d[51]!=1)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=52;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool ConvShape(const uint32_t* d,const uint32_t* table,unsigned w){
 if(!d || !table || w>=120 || !PioneerQ38ConvBatchShape(table) || table[10]!=w || table[9]!=(w?PioneerQ38MixedWorkerLayout::Peer(w):129)
  || table[5]!=d[3] || table[40]!=d[4] || table[6]!=d[5] || table[3]!=d[7] || table[4]!=d[8] || table[8]!=d[9])return false;
 for(unsigned i=0;i<8;i++)if(table[32+i]!=d[32+i])return false;
 if(!w)for(unsigned i=0;i<16;i++)if(table[16+i]!=d[16+i])return false;
 return true;
}
static inline bool GdnShape(const uint32_t* d,const uint32_t* record,const uint32_t* conv,unsigned w){
 if(!d || !record || !conv || !w || w>96)return false;
 uint32_t job[16];PioneerQ38MixedRecurrentDispatch::Internal(8,d[6],w,1,job);
 if(!PioneerQ38ResidentGdnServiceJob(job,record+128,record,record+64,conv+16,w,PioneerQ38MixedWorkerLayout::Peer(w))
  || record[5]!=d[3] || record[40]!=d[4] || record[6]!=d[6] || record[3]!=d[7] || record[4]!=d[8]
  || record[64+11]!=d[14])return false;
 for(unsigned i=0;i<8;i++)if(record[32+i]!=d[32+i])return false;
 return true;
}
static_assert(Conv+ConvWords*4<=Gdn && Gdn+GdnWords*4<=Counters && Counters+119*32<=Nonces
 && Nonces+119*4<=ConvShadow && ConvShadow+ConvWords*4==GdnShadow
 && GdnShadow+GdnWords*4<=PeerBoots && PeerBoots+119*256<=PeerJobs && PeerJobs+119*256==End,
 "NEW phased cohort tables preserve all retained mixed peer state");
static_assert(Header+256<=TT_Q38_CONV_BATCH_TABLE && TT_Q38_CONV_BATCH_TABLE+1536<=Boot
 && Boot+256<=Doorbell && Status+64<=Summary && Summary+256<=Scratch,"ROOT mailboxes and numeric capsule table disjoint");
}
#endif
