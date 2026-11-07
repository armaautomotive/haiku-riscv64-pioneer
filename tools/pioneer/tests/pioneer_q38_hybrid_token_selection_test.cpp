/* SPDX-License-Identifier: MIT */
// Distinct test-only selection of QGW5 footprint with QTP4 fixture DATA.
#include "../PioneerQ38TokenFootprint.h"
static_assert(PioneerQ38TokenL1::Family==0x35505451,"hybrid fixture must not alias old QTP4 geometry");
static_assert(PioneerQ38TokenL1::SavedBytes==137812488,"complete proposed QGW5 memory journal");
int main(){return PioneerQ38TokenL1::SnapshotContains(119,0x170000)?0:1;}
