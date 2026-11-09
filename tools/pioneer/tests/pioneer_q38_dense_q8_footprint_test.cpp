/* SPDX-License-Identifier: MIT */
// Geometry/alias arithmetic only; no live mapping, save/restore or device.
#define PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION 1
#define PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC 1
#include "../PioneerQ38DenseQ8TokenFootprint.h"
#include <cassert>
#include <cstdio>
namespace D=PioneerQ38DenseQ8TokenL1;namespace H=PioneerQ38HybridTokenL1;
static bool Alias(uint32_t a){
 if(a==0x6000)return true;
 if(a>=0x20000 && a<0x60000)return true;
 if(a>=0x60000 && a<0x6e000)return true;
 if(a>=0x6e000 && a<0xac000)return true;
 if(a>=0xac000 && a<0x17ffc0)return true;
 return a>=0x17ffc0 && a<0x180000;
}
int main(){
 for(unsigned worker=0;worker<120;worker++){
  unsigned total=0;uint32_t end=0;
  for(unsigned i=0;i<D::SnapshotSpans(worker);i++){
   const auto s=D::SnapshotSpan(worker,i),old=H::SnapshotSpan(worker,i);
   assert(s.address==old.address && s.words>=old.words && (!i || s.address>=end));
   assert(s.address<=UINT32_MAX-s.words*4);end=s.address+s.words*4;
   assert(Alias(s.address) && Alias(end-4));
   assert(D::SnapshotAddress(worker,total)==s.address && D::SnapshotAddress(worker,total+s.words-1)==end-4);
   assert(D::SnapshotContains(worker,old.address) && D::SnapshotContains(worker,old.address+(old.words-1)*4));
   total+=s.words;
  }
  assert(total==D::SnapshotWords(worker) && D::SnapshotAddress(worker,total)==UINT32_MAX);
  if(worker){
   assert(D::SnapshotSpan(worker,1).words==4096 && D::SnapshotContains(worker,0x20000+13148-4));
   assert(!H::SnapshotContains(worker,0x20000+13148-4) && !D::SnapshotContains(worker,0x24000));
  }
 }
 assert(D::SnapshotBase(119)+D::SnapshotWords(119)==D::SavedWords && D::SnapshotWords(120)==0);
 assert(D::SnapshotBase(120)==UINT32_MAX && !D::SnapshotContains(120,0x20000));
 std::printf("PASS QGWB proposed journal %u bytes; old spans and new code covered, alias geometry only; NO NATIVE OR DEVICE\n",D::SavedBytes);
}
