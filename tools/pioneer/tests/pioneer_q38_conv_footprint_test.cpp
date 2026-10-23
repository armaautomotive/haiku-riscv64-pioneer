/* SPDX-License-Identifier: MIT */
// Match QCV1's ACTUAL emitted bytes/fixed L1 buffers to the EXISTING full
// projection save/restore footprint. This does NOT transfer ownership,
// overwrite a live image,or make old Image() proofs accept new code.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvShape.h"
#include <cassert>
#include <cstdio>
#include <sys/stat.h>
int main(int argc,char** argv){
 assert(argc==2);struct stat file={};assert(!stat(argv[1],&file) && S_ISREG(file.st_mode) && file.st_size==5360);
 unsigned words=0;
 const PioneerQ38ProjectionGraphSpan data[]={{TT_Q38_FFN_RESIDENT_DESCRIPTOR,64},{TT_Q38_FFN_RESIDENT_SUMMARY,32},
  {0x80000,16},{0x80400,64},{0x80800,48},{0x80c00,48},{0x81000,16}};
 for(unsigned w=0;w<120;w++){
  assert(PioneerQ38ProjectionGraphSnapshotContains(w,0x6000) && PioneerQ38ProjectionGraphPrefixBytes(w)>=unsigned(file.st_size));
  for(unsigned a=0x64000;a<0x64000+unsigned(file.st_size);a+=4){assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));words++;}
  for(const auto& span:data)for(unsigned i=0;i<span.words;i++){
   unsigned a=span.address+i*4;assert(PioneerQ38ProjectionGraphSnapshotContains(w,a) && !PioneerQ38ProjectionGraphProgramImmutable(w,a));words++;
  }
  for(unsigned a=0x6e000;a<0x70000;a+=4){assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));words++;}
  for(unsigned a=0x17ffc0;a<0x17ffcc;a+=4){assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));words++;}
  // Prospective6job on-card batch table can use ALREADY-saved mutable
  // scratch,not a new unsaved address/constant image/stack/summary overlap.
  for(unsigned a=0x74800;a<0x74e00;a+=4)assert(PioneerQ38ProjectionGraphSnapshotContains(w,a) && !PioneerQ38ProjectionGraphProgramImmutable(w,a));
 }
 printf("PASS QCV1 existing L1 footprint ALL120cores/%uactual code+data+stack+trailer words;prospective6job table1536B covered;NO snapshot growth/native owner handoff/launch/restore authority\n",words);
}
