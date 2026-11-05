/* SPDX-License-Identifier: MIT */
// LOCAL VALIDATION ONLY. Independent vendor-derived bit-perfect FMA model.
#include "../PioneerBlackholeFma.h"
#include <cstdio>
#include <initializer_list>
int main(){
 unsigned cancellations=0,integers=0;
 for(int sum=-2048;sum<=2048;sum++){
  const unsigned offset=PioneerFmaBits(float(sum*7)),dot=PioneerFmaBits(float(-sum*7));
  assert(PioneerBlackholeFmaBits(0x3f800000,dot,offset)==0);cancellations++;
 }
 for(unsigned sign:{0U,0x80000000U})assert(PioneerBlackholeFmaBits(0x3f800000,sign,0)==0);
 for(int partial=-16384;partial<=16384;partial++)for(int sum:{-2048,0,2048}){
  const unsigned expected=PioneerFmaBits(float(partial+7*sum));
  assert(PioneerBlackholeFmaBits(0x3f800000,PioneerFmaBits(float(partial)),PioneerFmaBits(float(7*sum)))==expected);integers++;
 }
 // Necessity:without the required restoration,raw -0 is NOT canonical.
 assert(PioneerFmaBits(-0.0f)==0x80000000);
 puts("PASS_LOCAL_TWO_PRODUCT_RESTORE_ZERO_PROOF exact_integer_domain_below2pow15 all4097_cancellations both_raw_zero_signs positive_offset_zero;not_a_generic_zero_elision");
 printf("RESTORE_ZERO cases%u integer_extremes%u;NO_NATIVE_HARDWARE_OR_TPS\n",cancellations,integers);
}
