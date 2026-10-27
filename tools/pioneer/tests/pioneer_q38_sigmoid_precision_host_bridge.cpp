/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "../PioneerQ38SigmoidPrecision.h"
#include "pioneer_q38_sigmoid_precision_host_bridge.h"
bool ProbeQ38SigmoidPrecision(uint32_t input,uint32_t* output){
 if(!output)return false;
 float value;if(!PioneerQ38SigmoidPrecision(PioneerAttentionFloat(input),value))return false;
 *output=PioneerAttentionBits(value);return true;
}
