/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q6_UNPACK_H
#define PIONEER_Q38_NATIVE_Q6_UNPACK_H
#include "PioneerQ38NativeQ6Tiles.h"
#include "PioneerQ38TwinNativeQ8.h"

// Scoped native-INT8 command sequence only. The worker must already
// own the tensor engines, admit the exact new role and retire a checked
// transfer of this complete SM7 tile before loading it. Metadata is not
// an INT8 operand. Temporary configuration is the same as native Q8.
namespace PioneerQ38NativeQ6Unpack {
template<class E> static void Enter(E& e){PioneerQ38TwinNativeQ8::Enter(e);}
template<class E> static void Restore(E& e){PioneerQ38TwinNativeQ8::Restore(e);}

template<class E> static bool Panel(E& e,unsigned quantType,unsigned rawTile,unsigned panel){
 unsigned address=0;
 if(quantType!=14 || !PioneerQ38NativeQ6Tiles::TwinFetch::PanelAddress(rawTile,panel,address))return false;
 e.Drain();e.Config(76,(address-16)/16);
 e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
 e.Op(0x420000c1);e.Drain();return true;
}
}
#endif
