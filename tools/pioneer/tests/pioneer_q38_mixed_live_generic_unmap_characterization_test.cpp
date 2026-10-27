/* SPDX-License-Identifier: MIT */
// Characterize the existing diagnostic method,not a failing regression:
// it is intentionally preserved for old roles,unsuitable for NEW strict END.
#include "PioneerQ38MixedFinalUnmapFixture.h"
int main(){
 for(unsigned fail=0;fail<=7;fail++){
  removed.clear();failDelete=fail;inheritedHardwareCleanup=0;FinalUnmapFixture io;
  assert(io.Unmap()==!fail && removed.size()==7 && inheritedHardwareCleanup==1);
  // Four NEW mixed aliases are deferred to destruction by the generic
  // method. Failure of an early deletion does NOT fence later deletions.
  for(unsigned i=0;i<4;i++)assert(io.q38MixedLiveMemoryArea[i]==int(100+i));
 }
 puts("REPRODUCED actual generic SessionIO Unmap:7 direct VM deletions+inherited diagnostic cleanup continue after early deletion failure;4 mixed views deferred to destruction;old behavior preserved,NOT qualified for new first-failure final END");
}
