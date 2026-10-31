/* SPDX-License-Identifier: MIT */
// HOST-only execution of the actual new input math, no CPU oracle role.
#define PIONEER_Q38_GDN_INPUT_IMAGE 1
#define PIONEER_Q38_GDN_PREPARE_IMAGE 1
#define PIONEER_Q38_GDN_INPUT_COMBINED_STAGE 1
#define PIONEER_Q38_GDN_PRECISE_BETA_IMAGE 1
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "../PioneerQ38GdnPrepare.h"
#include "pioneer_q38_layer29_math_bridge.h"
bool Q38Layer29CardPreparePrecise(const float* q,const float* k,float a,float b,float negativeA,float dt,float* outQ,float* outK,float* gates){
 return PioneerQ38PrepareHead(q,k,a,b,negativeA,dt,outQ,outK,gates);
}
