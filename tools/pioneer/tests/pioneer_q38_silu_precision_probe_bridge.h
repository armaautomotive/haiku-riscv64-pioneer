/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SILU_PRECISION_PROBE_BRIDGE_H
#define PIONEER_Q38_SILU_PRECISION_PROBE_BRIDGE_H
#include <stdint.h>
// LOCAL emitted diagnostic only;no native producer/owner or device FD.
void ExecuteSiluPrecisionProbe(const char*,const uint32_t*,uint32_t*);
void ExecuteUnscaledPrepareProbe(const char*,unsigned,const uint32_t*,uint32_t*);
#endif
