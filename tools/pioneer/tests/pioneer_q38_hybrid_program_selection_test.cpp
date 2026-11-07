/* SPDX-License-Identifier: MIT */
// Offline selected-program and journal must be the same experimental family.
#include "../PioneerQ38TokenSelectedPrograms.h"
#include "../PioneerQ38SelectedGlobalWorker.h"
#ifdef PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
static_assert(PioneerQ38TokenPrograms::Family==0x37505451,"distinct QGW7 program DATA required");
static constexpr unsigned expectedWorker=0x37574751;
#else
static_assert(PioneerQ38TokenPrograms::Family==0x35505451,"distinct QGW5 program DATA required");
static constexpr unsigned expectedWorker=0x35574751;
#endif
static_assert(PioneerQ38TokenL1::Family==0x35505451,"distinct QGW5 journal required");
static_assert(PioneerQ38SelectedWorkerHybrid==1 && PioneerQ38SelectedWorkerBootEnd==51,
 "hybrid program DATA must select the matching startup grammar");
int main(){
 (void)&PioneerQ38SelectedGlobalWorker::JobShape;
 return PioneerQ38TokenPrograms::CodeSpans(119)==10
  && PioneerQ38SelectedGlobalWorker::Magic==expectedWorker
  && PioneerQ38SelectedGlobalWorker::BootWord(50)==0x1fff0000?0:1;
}
