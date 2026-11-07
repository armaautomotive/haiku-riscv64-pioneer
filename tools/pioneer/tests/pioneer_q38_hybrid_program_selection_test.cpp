/* SPDX-License-Identifier: MIT */
// Offline selected-program and journal must be the same experimental family.
#include "../PioneerQ38TokenSelectedPrograms.h"
static_assert(PioneerQ38TokenPrograms::Family==0x35505451,"distinct QGW5 program DATA required");
static_assert(PioneerQ38TokenL1::Family==0x35505451,"distinct QGW5 journal required");
int main(){return PioneerQ38TokenPrograms::CodeSpans(119)==10?0:1;}
