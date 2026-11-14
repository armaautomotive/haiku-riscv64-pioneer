/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION) || PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(_KERNEL_MODE)
#error Offline recurrent-state L1 capacity inspection only
#endif
#ifndef PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION_H
#define PIONEER_Q38_GDN_L1_CACHE_CAPACITY_INSPECTION_H
#include "PioneerQ38TokenPushedFootprint.h"
#include <stdint.h>
namespace PioneerQ38GdnL1CacheCapacityInspection {
enum {L1Bytes=0x180000,BlockBytes=64,Blocks=L1Bytes/BlockBytes,
 StateBytes=32768,BlocksPerState=StateBytes/BlockBytes,MaxSlots=64,
 PageBytes=4096,BlocksPerPage=PageBytes/BlockBytes,PagesPerState=StateBytes/PageBytes,
 MaxPages=L1Bytes/PageBytes};
struct Slot {uint32_t address,bytes;};
struct FragmentedSlot {uint32_t pages[PagesPerState];};
static inline bool Free(unsigned worker,uint32_t address,uint32_t bytes)
{
 if(worker==0 || worker>=PioneerQ38TokenPushedL1::Workers || !bytes || (address&63)
    || address>L1Bytes || bytes>L1Bytes-address)return false;
 for(unsigned i=0;i<PioneerQ38TokenPushedL1::SnapshotSpans(worker);i++){
  const auto s=PioneerQ38TokenPushedL1::SnapshotSpan(worker,i);
  const uint32_t spanBytes=s.words*4;
  if(address<s.address+spanBytes && s.address<address+bytes)return false;
 }
 return true;
}
static inline bool Build(unsigned worker,Slot* out,unsigned capacity,unsigned& count)
{
 count=0;if(!out || capacity>MaxSlots || worker==0 || worker>=PioneerQ38TokenPushedL1::Workers)return false;
 bool reserved[Blocks]={};
 for(unsigned i=0;i<PioneerQ38TokenPushedL1::SnapshotSpans(worker);i++){
  const auto s=PioneerQ38TokenPushedL1::SnapshotSpan(worker,i);
  const uint32_t first=s.address/BlockBytes;
  const uint32_t end=(s.address+s.words*4+BlockBytes-1)/BlockBytes;
  if(end>Blocks)return false;
  for(uint32_t b=first;b<end;b++)reserved[b]=true;
 }
 for(unsigned first=0;first<Blocks;){
  while(first<Blocks && reserved[first])first++;
  unsigned end=first;while(end<Blocks && !reserved[end])end++;
  const unsigned slots=(end-first)/BlocksPerState;
  for(unsigned i=0;i<slots;i++){
   if(count>=capacity)return false;
   const uint32_t address=(first+i*BlocksPerState)*BlockBytes;
   if(!Free(worker,address,StateBytes))return false;
   out[count++]={address,StateBytes};
  }
  first=end;
 }
 return true;
}
static inline bool BuildPages(unsigned worker,uint32_t* out,unsigned capacity,unsigned& count)
{
 count=0;if(!out || capacity>MaxPages || worker==0 || worker>=PioneerQ38TokenPushedL1::Workers)return false;
 bool reserved[Blocks]={};
 for(unsigned i=0;i<PioneerQ38TokenPushedL1::SnapshotSpans(worker);i++){
  const auto s=PioneerQ38TokenPushedL1::SnapshotSpan(worker,i);
  const uint32_t first=s.address/BlockBytes,end=(s.address+s.words*4+BlockBytes-1)/BlockBytes;
  if(end>Blocks)return false;
  for(uint32_t b=first;b<end;b++)reserved[b]=true;
 }
 for(unsigned first=0;first<Blocks;){
  while(first<Blocks && reserved[first])first++;
  unsigned end=first;while(end<Blocks && !reserved[end])end++;
  unsigned page=(first+BlocksPerPage-1)&~(BlocksPerPage-1);
  while(page+BlocksPerPage<=end){
   if(count>=capacity)return false;
   out[count]=page*BlockBytes;
   if(!Free(worker,out[count],PageBytes))return false;
   count++;page+=BlocksPerPage;
  }
  first=end;
 }
 return true;
}
static inline bool BuildFragmented(unsigned worker,FragmentedSlot* out,unsigned capacity,
 unsigned& count,unsigned& pageCount)
{
 uint32_t pages[MaxPages];pageCount=0;count=0;
 if(!out || capacity>MaxSlots || !BuildPages(worker,pages,MaxPages,pageCount))return false;
 count=pageCount/PagesPerState;if(count>capacity)return false;
 for(unsigned i=0;i<count;i++)for(unsigned j=0;j<PagesPerState;j++)
  out[i].pages[j]=pages[i*PagesPerState+j];
 return true;
}
}
#endif
