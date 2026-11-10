/* SPDX-License-Identifier: MIT */
// Invalid-input checks only; fixture metadata is synthetic and cannot pass
// source authentication. The actual real-model proof is a separate test.
#include "../PioneerNativeModelBandSource.h"
#include <cassert>
#include <cfenv>
#include <string>
int main(){
 std::string error;PioneerNativeModelBandSource once;
 assert(!once.Ready() && !once.Prepare(nullptr,error) && !once.Ready() && !once.Prepare("/not/a/model",error));
 PioneerNativeModelBandSource missing;
 assert(!missing.Prepare("/not/a/model",error) && !missing.Ready());
 const int initial=std::fegetround();assert(!std::fesetround(FE_DOWNWARD));
 PioneerNativeModelBandSource rounding;assert(!rounding.Prepare("/not/a/model",error) && !rounding.Ready());
 assert(!std::fesetround(initial));
}
