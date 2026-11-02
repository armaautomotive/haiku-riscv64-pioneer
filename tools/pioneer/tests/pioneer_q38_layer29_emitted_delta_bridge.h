/* SPDX-License-Identifier: MIT */
#pragma once
#include <cstdint>
// HOST diagnostic DATA only. Key-major full-head state is packed into two
// independent, unchanged 64-column emitted local images. No native owner,
// NoC, model loader, runtime CPU reference or physical execution capability.
bool Q38Layer29EmittedDelta(const char*,const float*,const float*,const float*,
 float,float,float*,float*,uint64_t*,uint64_t*);
