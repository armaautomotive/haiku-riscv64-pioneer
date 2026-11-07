/* SPDX-License-Identifier: MIT */
// QGW5 footprint geometry and existing native-alias bounds, offline only.
#define PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC 1
#include "../PioneerQ38HybridTokenFootprint.h"
#include "../PioneerQ38Bfp4WorkerJob.h"
#include "../PioneerQ38Bfp8WorkerJob.h"
#include <cassert>
#include <cstdio>
namespace H=PioneerQ38HybridTokenL1;namespace Old=PioneerQ38TokenBf16L1;

// Independent shape of current TokenMemoryReadChecked/Write routing. This
// is only arithmetic on addresses, not evidence that mappings are active.
static bool ExistingAliasCovers(uint32_t a){
 if(a==0x6000)return true;
 if(a>=0x20000 && a<0x40000)return a-0x20000<131072;
 if(a>=0x40000 && a<0x60000)return a-0x40000<131072;
 if(a>=0x60000 && a<0x6e000)return a-0x60000<57344;
 if(a>=0x6e000 && a<0xac000)return a-0x6e000<0x42000;
 if(a>=0xac000 && a<0x17ffc0)return a-0xac000<868352;
 if(a>=0x17ffc0 && a<0x180000)return 0x1fc0+(a-0x17ffc0)<8192;
 return false;
}
static void Covered(unsigned worker,uint32_t start,uint32_t bytes){
 assert(!(start&3) && !(bytes&3));
 for(unsigned at=0;at<bytes;at+=4)assert(H::SnapshotContains(worker,start+at) && ExistingAliasCovers(start+at));
}
int main(){
 unsigned long long oldWords=0;
 for(unsigned worker=0;worker<120;worker++){
  unsigned count=0;uint32_t end=0;
  for(unsigned i=0;i<H::SnapshotSpans(worker);i++){
   const auto s=H::SnapshotSpan(worker,i);assert(s.words && !(s.address&3) && (!i || s.address>=end));
   assert(s.address<=UINT32_MAX-s.words*4);end=s.address+s.words*4;
   Covered(worker,s.address,s.words*4);
   assert(H::SnapshotAddress(worker,count)==s.address
    && H::SnapshotAddress(worker,count+s.words-1)==s.address+(s.words-1)*4);
   count+=s.words;
  }
  assert(count==H::SnapshotWords(worker));
  for(unsigned i=0;i<Old::SnapshotWords(worker);i++){
   const uint32_t a=Old::SnapshotAddress(worker,i);assert(H::SnapshotContains(worker,a));oldWords++;
  }
  assert(H::SnapshotAddress(worker,0)==0x6000
   && H::SnapshotAddress(worker,H::SnapshotWords(worker))==UINT32_MAX
   && !H::SnapshotContains(worker,0x6001));
  if(worker){
   Covered(worker,0x8ffc0,544*1152); // max BFP8 weight headers + slots
   Covered(worker,0x12bfc0,544*128); // max native input headers + slots
   Covered(worker,0x84000,8*32*4);   // actual result words
   Covered(worker,0x140000,256);      // conservative output reservation
   Covered(worker,0x170000,0xffc0);  // linked GDN code maximum
   Covered(worker,0x17ffc0,32);      // terminal/status
   assert(!H::SnapshotContains(worker,0x141000) && !H::SnapshotContains(worker,0x16fffc));
  }else{
   Covered(0,0x16c400,64*3*7*4);Covered(0,0x16da00,7*4); // ROOT manifest + shadow
  }
 }
 assert(oldWords==Old::SavedWords && H::SnapshotBase(119)+H::SnapshotWords(119)==H::SavedWords
  && H::SnapshotSpans(120)==0 && H::SnapshotAddress(120,0)==UINT32_MAX);
 printf("PASS OFFLINE QGW5 footprint/root manifest/all120 aliases/%llu previous words covered; "
  "peer%u total%u words %u bytes, NOT ownership, native mapping, restore or hardware execution\n",
  oldWords,H::PeerSavedWords,H::SavedWords,H::SavedBytes);
}
