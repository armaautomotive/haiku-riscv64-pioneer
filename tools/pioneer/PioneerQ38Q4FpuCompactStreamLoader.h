/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_COMPACT_STREAM_LOADER_H
#define PIONEER_Q38_Q4_FPU_COMPACT_STREAM_LOADER_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Q4 streaming prototype is not a driver/Work/reference capability
#endif
#include "PioneerQ38Q4FpuCompactStreamLayout.h"
#include "PioneerDramCohortReadSweep.h"
// OFFBOARD leaf,NOT native authority. Exclusive NIU/FIFO/L1/GDDR/clock/source
// lease required externally. Two disjoint buffers,one pending prefetch,one
// reader. Two13-request slices can overlap two groups of current math.
// No scalar payload read/copy,DRAM write,host callback,retry/reset/cleanup.
class PioneerQ38Q4FpuCompactStreamLoader {
	using L=PioneerQ38Q4FpuCompactStreamLayout;
#define PIONEER_Q38_Q4_STREAM_LOADER_CLASS PioneerQ38Q4FpuCompactStreamLoader
#include "PioneerQ38Q4StreamLoaderBody.inc"
#undef PIONEER_Q38_Q4_STREAM_LOADER_CLASS
};
#endif

