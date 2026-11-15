/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4 1
#define PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4 1
#define PIONEER_Q38_FULL_OFFLOAD_BATCHED 1
#define PIONEER_Q38_FULL_OFFLOAD_METRICS 1
#define PIONEER_Q38_FULL_OFFLOAD_CLIENT 1
#include "../PioneerQ38TokenPushedFootprint.h"
#include <cassert>
namespace L=PioneerQ38TokenPushedL1;
int main(){
 assert(L::SpillQ4==1 && L::PreparedQ4==0 && L::FastDecode==0);
 assert(L::SnapshotSpans(0)==8 && L::SnapshotWords(0)==321033);
 assert(L::RootSpillWords==4094 && L::SavedBytes==53930208);
 const auto spill=L::SnapshotSpan(0,3);
 assert(spill.address==0x60008 && spill.words==4094);
 assert(L::SnapshotAddress(0,32771)==0x60008 && L::SnapshotAddress(0,36865)==0x64000);
 assert(L::SnapshotContains(0,0x60000) && L::SnapshotContains(0,0x60008)
  && L::SnapshotContains(0,0x63ffc) && !L::SnapshotContains(0,0x64000-1)
  && L::SnapshotContains(0,0x64000) && !L::SnapshotContains(0,0x64000+32768));
 assert(L::SnapshotSpans(1)==8 && L::SnapshotContains(1,0x60008));
 assert(L::SnapshotWords(120)==0 && !L::SnapshotContains(120,0x60008));
}
