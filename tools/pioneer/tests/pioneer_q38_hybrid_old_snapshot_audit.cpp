/* SPDX-License-Identifier: MIT */
// Show why QGW4's existing peer journal cannot safely host QGW5. This is a
// read-only geometry audit; it does not create or exercise a native journal.
#include "../PioneerQ38TokenBf16Footprint.h"
#include "../PioneerQ38Bfp8WorkerJob.h"
#include "../PioneerNativeBfp4DramBandLayout.h"
#include "../PioneerQ38DensePartitionLayout.h"
#include <cassert>
#include <cstdio>
namespace Old=PioneerQ38TokenBf16L1;
static unsigned Missing(unsigned worker,unsigned begin,unsigned end){
 assert(!(begin&3) && !(end&3) && begin<end);
 unsigned count=0;for(unsigned a=begin;a<end;a+=4)count+=!Old::SnapshotContains(worker,a);
 return count;
}
int main(){
 using B=PioneerQ38Bfp8WorkerLayout;using A=PioneerNativeBfp4DramBandLayout;
 const unsigned start=B::Weight-B::Header,end=B::Weight-B::Header+544*B::WeightStride;
 const unsigned input=B::Input-B::Header,inputEnd=input+544*B::InputStride;
 const unsigned gdn=0x170000,gdnEnd=0x179000;
 assert(start<end && end<=input && inputEnd<=B::Output && B::Output<gdn);
 assert(A::Weight-A::Header==start && A::WeightEnd(17408)<=end);
 // Dense scores intentionally share the temporary stage. Separate phases
 // and actual no-outstanding-writer/consumer proofs remain mandatory.
 assert(PioneerQ38DensePartitionLayout::Scores>=start && PioneerQ38DensePartitionLayout::Scores<end);
 const unsigned missingWeights=Missing(1,start,end),missingInput=Missing(1,input,inputEnd),missingGdn=Missing(1,gdn,gdnEnd);
 assert(missingWeights && missingInput==(inputEnd-input)/4 && missingGdn==(gdnEnd-gdn)/4);
 for(unsigned worker=2;worker<120;worker++){
  assert(Old::SnapshotSpans(worker)==Old::SnapshotSpans(1));
  for(unsigned i=0;i<Old::SnapshotSpans(1);i++){
   const auto a=Old::SnapshotSpan(worker,i),b=Old::SnapshotSpan(1,i);
   assert(a.address==b.address && a.words==b.words);
  }
 }
 printf("OLD JOURNAL INSUFFICIENT per peer: %u BFP8 weight-stage words, %u input-stage words, %u relocated-GDN words unsaved; "
  "same old snapshot geometry for all119 peers; dense scratch shares staging; NO HARDWARE\n",missingWeights,missingInput,missingGdn);
}
