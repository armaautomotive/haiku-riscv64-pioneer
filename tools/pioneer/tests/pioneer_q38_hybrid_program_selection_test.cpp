/* SPDX-License-Identifier: MIT */
// Offline selected-program and journal must be the same experimental family.
#include "../PioneerQ38TokenSelectedPrograms.h"
#include "../PioneerQ38SelectedGlobalWorker.h"
static_assert(PioneerQ38TokenPrograms::Family==0x35505451,"distinct QGW5 program DATA required");
static_assert(PioneerQ38TokenL1::Family==0x35505451,"distinct QGW5 journal required");
static_assert(PioneerQ38SelectedWorkerHybrid==1 && PioneerQ38SelectedWorkerBootEnd==51,
 "QGW5 program DATA must select QGW5 startup grammar");
int main(){
 (void)&PioneerQ38SelectedGlobalWorker::JobShape;
 return PioneerQ38TokenPrograms::CodeSpans(119)==10
  && PioneerQ38SelectedGlobalWorker::Magic==0x35574751
  && PioneerQ38SelectedGlobalWorker::BootWord(50)==0x1fff0000?0:1;
}
