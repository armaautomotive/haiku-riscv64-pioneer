/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38TokenPushedFootprint.h"
#include <cassert>
#include <cstdio>
int main()
{
 namespace F=PioneerQ38TokenPushedL1;
 unsigned deltaWords=0,romWords=0;
 for(unsigned worker=1;worker<120;worker++){
  const auto data=F::SnapshotSpan(worker,5),rom=F::SnapshotSpan(worker,6);
  assert(data.address==0x70000 && data.words==61440 && rom.address==0xb8000 && rom.words==9216);
  for(unsigned i=0;i<64;i++){assert(F::SnapshotContains(worker,0x8e600+i*4));deltaWords++;}
  for(unsigned i=0;i<35944/4;i++){assert(F::SnapshotContains(worker,0xb8000+i*4));romWords++;}
  assert(35944<=rom.words*4 && rom.words*4-35944==920);
  assert(!F::SnapshotContains(worker,0xc1000) && !F::SnapshotContains(worker,0x8e601));
 }
 assert(F::SavedWords==13478458 && F::SavedBytes==53913832);
 printf("PASS GEOMETRY ONLY existing QTP3 journal/%u new Delta words+%u new ROM words covered/920B GDN padding remains/NO range expansion or saved-count change;NOT new image,owner,map,source,launch,closure or native capability\n",deltaWords,romWords);
}
