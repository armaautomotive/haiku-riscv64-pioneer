/* SPDX-License-Identifier: MIT */
// Actual Haiku SDK/type/map API compile of the complete adapter fragment.
// No genuine source factory or Driver/IO role is linked/selected here.
#define PIONEER_Q38_RESIDENT_OUTPUT_OWNER 1
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#include <KernelExport.h>
#include <stdint.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputFootprint.h"
enum {TT_Q38_PANEL_VIEW_BASE=0x6e000,TT_Q38_PANEL_VIEW_BYTES=0x42000};
uint32 PioneerRead32(const volatile uint32*);
void PioneerWrite32(volatile uint32*,uint32);
struct ResidentOutputNativeMappingCompileIO {
 struct {uint64 bar0_host_start;}profile;
 unsigned runtimeCore,payloadBytes;bool fleetEnabled,runtimeMapped;
 volatile uint32* runtimeEntry;volatile uint32* scratchData;
 bool ResidentOutputMappingSourceScope();bool Q38PanelMappingStable();
 uint32 RuntimeCodeRead(unsigned);void RuntimeCodeWrite(unsigned,uint32);
 uint32 Q38PanelRead(uint32);void Q38PanelWrite(uint32,uint32);
 uint32 ResidentL1Read(unsigned);void ResidentL1Write(unsigned,uint32);
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeMapping.inc"
};
extern "C" bool pioneer_output_mapping_kernel_compile(ResidentOutputNativeMappingCompileIO& io,unsigned w,uint32 a,uint32& out,uint32 value){
 return io.MapResidentOutputMemory() && io.ResidentOutputMemoryMappingStable(w)
  && io.ResidentOutputMemoryReadChecked(w,a,out) && io.ResidentOutputMemoryWrite(w,a,value);
}
