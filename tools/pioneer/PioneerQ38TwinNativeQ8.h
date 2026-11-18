/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_NATIVE_Q8_H
#define PIONEER_Q38_TWIN_NATIVE_Q8_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Twin-buffer native Q8 is a separate card-code inspection candidate
#endif
#include "PioneerQ38TwinBatchPrefetchPlan.h"

// Scoped DATA+instruction candidate, not a replacement for the returning
// worker. The caller must bind a distinct authenticated SM7 source, retain
// its existing completed twin-batch lease, and already own both unpack
// engines and Dst. No NoC/MMIO address, role or output authority is exposed.
namespace PioneerQ38TwinNativeQ8 {
using T=PioneerQ38TwinBatchPrefetchPlan;
enum : unsigned { Tile=576, Header=32, PanelBytes=256, Q8Type=8 };
static_assert(Header+2*PanelBytes+32==Tile && (Header&31)==0,
 "same byte-major sixteen-row tile, only sign format changes");

static constexpr bool PanelAddress(unsigned rawTile,unsigned panel,unsigned& address)
{
 if(panel>=2 || rawTile<T::Buffer0 || rawTile>=T::BufferLimit)return false;
 const unsigned offset=(rawTile-T::Buffer0)&(T::BufferStride-1);
 static_assert((T::BufferStride&(T::BufferStride-1))==0
  && T::Buffer1-T::Buffer0==T::BufferStride && 4*Tile<=T::BufferStride
  && T::Buffer1+4*Tile<=T::BufferLimit,"two exact four-tile batches");
 if(offset!=0 && offset!=Tile && offset!=2*Tile && offset!=3*Tile)return false;
 address=rawTile+Header+panel*PanelBytes;return true;
}

// Separate job setup/restore. The old Q4, Q6 and checked-Q8 paths must
// keep their original decoder and formats. Register writes and waits here
// follow the previously qualified single-buffer native SM7 procedure;
// retaining the direct SrcB path and affine metadata switch is still the
// parent integration's responsibility.
template<class E> static void Enter(E& e)
{
 e.Drain();e.Config(0,0x423e);e.Config(1,0xa03c0000);e.Config(73,3);
 e.Op(0x8a000002);e.Drain();
}
template<class E> static void Restore(E& e)
{
 e.Drain();e.Config(0,0x4231);e.Config(1,0xa03c8000);e.Config(73,0x13);e.Drain();
}

// Only two 256-byte panels of an authenticated COMPLETE SM7 tile in one
// of the eight exact Q8 slots. The producer still checks readiness,
// command/counter closure, bank/lifetime, and the active role before this
// function. A successful return has drained the one native SrcA unpack.
template<class E> static bool Unpack(E& e,unsigned type,unsigned rawTile,unsigned panel)
{
 unsigned at=0;
 if(type!=Q8Type || !PanelAddress(rawTile,panel,at))return false;
 e.Drain();e.Config(76,(at-16)/16);
 e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
 e.Op(0x420000c1);e.Drain();return true;
}
}
#endif
