/* SPDX-License-Identifier: MIT */
// DATA statistics only. Rechecked outside the guard;not runtime inference.
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(__FAST_MATH__)
#error Escape-panel scan is host DATA inventory only,not native/Work/reference inference
#endif
#ifndef PIONEER_Q38_Q4_ESCAPE_PANEL_SCAN_H
#define PIONEER_Q38_Q4_ESCAPE_PANEL_SCAN_H
#include <cstdint>
struct PioneerQ38Q4EscapePanelScan {
 // Sixteen ORIGINAL row-major Q4_K rows,each with blocks 144B records.
 // Result bit p means at least one q==15 in that K16xN16 panel. No
 // dequantization,FP products,model context,cache creation or device IO.
 static bool Mask(const uint8_t* rows,unsigned bytes,unsigned blocks,unsigned block,
  unsigned& output,unsigned& inspected){
  if(!rows || (blocks!=20 && blocks!=24 && blocks!=68) || block>=blocks || bytes!=blocks*16*144)return false;
  unsigned mask=0,steps=0;
  for(unsigned p=0;p<16;p++){
   const unsigned offset=16+(p/4)*32+(p&1)*16,shift=(p&2)?4:0;
   bool found=false;
   for(unsigned n=0;n<16 && !found;n++){
    const auto* q=rows+(n*blocks+block)*144+offset;
    for(unsigned k=0;k<16;k++){
     steps++;if(((q[k]>>shift)&15)==15){found=true;break;}
    }
   }
   if(found)mask|=1U<<p;
  }
  output=mask;inspected=steps;return true;
 }
};
#endif
