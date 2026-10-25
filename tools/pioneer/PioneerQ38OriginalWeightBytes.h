/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ORIGINAL_WEIGHT_BYTES_H
#define PIONEER_Q38_ORIGINAL_WEIGHT_BYTES_H
#include <stdint.h>
// Authenticated original BYTES only. No numeric reference products,
// device IO,ownership,initialization,readiness or cleanup capability.
struct ProjectionExecutionLearnedBytes {
 virtual ~ProjectionExecutionLearnedBytes()=default;
 virtual bool Ready()const=0;
 virtual bool Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes)=0;
};
#endif
