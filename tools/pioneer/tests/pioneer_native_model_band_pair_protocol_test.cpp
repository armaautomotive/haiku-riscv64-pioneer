/* SPDX-License-Identifier: MIT */
#include <cstdint>
#include <cassert>
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerNativeModelBandPairProtocol.h"
int main(){
 PioneerNativeModelBandPairRequest r={};r.version=2;r.expected_tag=PioneerNativeModelBandPairTag;r.expected_generation=1;
 assert(PioneerNativeModelBandPairRequestValid(r));
 auto bad=r;bad.version=1;assert(!PioneerNativeModelBandPairRequestValid(bad));
 bad=r;bad.version=3;assert(!PioneerNativeModelBandPairRequestValid(bad));
 bad=r;bad.expected_tag^=1;assert(!PioneerNativeModelBandPairRequestValid(bad));
 bad=r;bad.expected_generation=0;assert(!PioneerNativeModelBandPairRequestValid(bad));
 bad=r;bad.overlap_verified=1;assert(!PioneerNativeModelBandPairRequestValid(bad));
 bad=r;bad.overlap_pc[3]=0x70010;assert(!PioneerNativeModelBandPairRequestValid(bad));
 for(unsigned i=0;i<2;i++){
  bad=r;bad.result[i].state=1;assert(!PioneerNativeModelBandPairRequestValid(bad));
  bad=r;bad.result[i].output[0]=1;assert(!PioneerNativeModelBandPairRequestValid(bad));
  bad=r;bad.result[i].output[63]=1;assert(!PioneerNativeModelBandPairRequestValid(bad));
 }
 assert(PIONEER_NATIVE_MODEL_BAND_PAIR_SUPPORT!=PIONEER_NATIVE_MODEL_BAND_PAIR_RUN);
 assert(PIONEER_NATIVE_MODEL_BAND_PAIR_RUN!=PIONEER_NATIVE_MODEL_BAND_PAIR_END);
}
