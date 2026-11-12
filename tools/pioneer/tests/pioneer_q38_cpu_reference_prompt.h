/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CPU_REFERENCE_PROMPT_H
#define PIONEER_Q38_CPU_REFERENCE_PROMPT_H
// CPU-only validation input policy. Generated IDs are never supplied to Work.
static inline unsigned PioneerQ38CpuReferenceInput(unsigned position,unsigned previous,bool autoregressive){
 return position==0?248319U:position==1 && !autoregressive?17U:previous;
}
#endif
