/* SPDX-License-Identifier: MIT */
// TEST transport DATA scope only. Include as members of the actual virgin
// native ROOT and of its bounded endpoint test. Never execute/supply code.
static bool ConvScratchPacket(bool active,uint32_t pc,uint32_t local,uint32_t bytes){
 uint32_t instruction=0;
 return active && pc>=0x64000 && pc<0x6c000 && !((pc|local|bytes)&3)
  && local>=0x80000 && local<0x81100 && bytes && bytes<=0x81100-local
  && PioneerQ38TokenPrograms::ProgramWord(0,pc,instruction);
} // EXACT selected installed auxiliary image;not padding,stack or arbitrary L1
