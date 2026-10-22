/* SPDX-License-Identifier: MIT */
#include <KernelExport.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
static uint32 PioneerRead32(const volatile uint32* p){return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 value){*p=value;}
enum {TT_Q38_PANEL_VIEW_BASE=0x6e000,TT_Q38_PANEL_VIEW_BYTES=0x42000};
struct NativeProjectionMemoryKernelCompile {
 struct {uint64 bar0_host_start;} profile;
 unsigned runtimeCore;bool fleetEnabled,runtimeMapped;volatile uint32* runtimeEntry;volatile uint32* scratchData;unsigned payloadBytes;
 area_id q38ProjectionMemoryArea[3];volatile uint32* q38ProjectionMemoryView[3];bool q38ProjectionMemoryMapAttempted;uint64 q38ProjectionMemoryBar;
 bool Q38PanelMappingStable();uint32 RuntimeCodeRead(unsigned);void RuntimeCodeWrite(unsigned,uint32);
 uint32 Q38PanelRead(uint32);void Q38PanelWrite(uint32,uint32);uint32 ResidentL1Read(unsigned);void ResidentL1Write(unsigned,uint32);
#include PIONEER_PROJECTION_MEMORY_ADAPTER_INCLUDE
};
extern "C" bool pioneer_projection_native_memory_map(NativeProjectionMemoryKernelCompile& io){return io.MapProjectionGraphMemory();}
extern "C" bool pioneer_projection_native_memory_read(NativeProjectionMemoryKernelCompile& io,unsigned w,uint32 a,uint32& out){return io.ProjectionGraphMemoryReadChecked(w,a,out);}
extern "C" bool pioneer_projection_native_memory_write(NativeProjectionMemoryKernelCompile& io,unsigned w,uint32 a,uint32 value){return io.ProjectionGraphMemoryWrite(w,a,value);}
