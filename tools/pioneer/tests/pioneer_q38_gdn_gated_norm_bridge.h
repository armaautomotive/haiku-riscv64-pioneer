/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_GATED_NORM_BRIDGE_H
#define PIONEER_Q38_GDN_GATED_NORM_BRIDGE_H
#include <stdint.h>
// TEST LOCAL byte endpoint for actual emitted instructions,not a native
// source/read/producer grant. No numeric references supplied to machine.
void ExecuteGdnGatedNormLocal(const char*,unsigned,const uint32_t*,const uint32_t*,const uint32_t*,uint32_t*);
#endif
