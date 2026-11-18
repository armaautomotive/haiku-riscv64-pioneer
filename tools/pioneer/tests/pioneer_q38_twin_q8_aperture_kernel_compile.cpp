/* SPDX-License-Identifier: MIT */
// This object must not link as a driver. Instantiate the new adapter against
// the real Pioneer driver types and actual Haiku kernel symbols, not a fake IO.
#define api_version q8_aperture_compile_only_api
#define init_hardware q8_aperture_compile_only_hardware
#define init_driver q8_aperture_compile_only_init
#define uninit_driver q8_aperture_compile_only_uninit
#define publish_devices q8_aperture_compile_only_publish
#define find_device q8_aperture_compile_only_find
#define PIONEER_NATIVE_BFP_DIAGNOSTIC 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TwinQ8NativeAperture.h"

struct Q8CompileOnlyOwner {
 bool Scope();bool WorkerStopProved(); // deliberately undefined
};
#ifdef PIONEER_Q38_TWIN_Q8_COMPILE_PAYLOAD
#include "PioneerQ38TwinQ8Payload.h"
using Q8CompileOnlyRom=PioneerQ38TwinQ8Payload::ROM;
extern "C" const uint32_t* q8_aperture_compile_only_weight_words(PioneerQ38TwinQ8ProjectionSequence::Arm arm)
{
 return Q8CompileOnlyRom{}.WeightWords(arm);
}
#else
struct Q8CompileOnlyRom {
 bool Section(PioneerQ38TwinQ8ProjectionSequence::Arm,unsigned,const uint32_t*&,unsigned)const;
};
#endif
namespace L=PioneerQ38TwinQ8L1Lease;
namespace A=PioneerQ38TwinQ8NativeAperture;
namespace S=L::S;
using Access=A::Access<Q8CompileOnlyOwner>;
extern "C" bool q8_aperture_compile_only(Access& io,L::Lease& lease,S::Arm arm,const S::Binding& bind,const Q8CompileOnlyRom& rom)
{
 uint32_t v=0;
 return io.MapHeld() && lease.Capture(io) && lease.Install(io,rom,arm,bind)
   && lease.NoteRelease() && io.NoteRelease()
   && io.Read(L::Tile::Worker,S::Status,v) && io.Write(L::Tile::Worker,S::Bell,1)
   && io.Count(L::Tile::Root,S::Slave(0),v) && lease.Restore(io) && io.ReleaseRestoredViews(lease);
}
