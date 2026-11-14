/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include "PioneerQ38TokenFfnSharedInputReusePrograms.h"

int main(){
 using namespace PioneerQ38TokenFfnSharedInputReusePrograms;
 static_assert(Family==0x33505451 && FfnSharedInputReuse==1,"isolated QTP3 candidate");
 assert(CodeSpans(0)==4 && CodeSpans(1)==9 && CodeSpans(120)==0);
 assert(CodeSpan(0,0).address==0x6000 && CodeSpan(0,1).address==0x40000);
 assert(CodeSpan(1,1).address==0x20000 && CodeSpan(1,2).address==0x30000
  && CodeSpan(1,3).address==0x40000);
 assert(CodeWords(0)==(4+131040+8+32708)/4);
 assert(CodeWords(1)==(4+9148+8392+10332+16456+9888+5360+5984+35564)/4);
 uint32_t word=0;
 assert(ProgramWord(0,0x6000,word) && word==RootBoot[0]);
 assert(ProgramWord(0,0x40000,word) && word==RootText[0]);
 assert(ProgramWord(1,0x20000,word) && word==PeerFront[0]);
 assert(ProgramWord(1,0x30000,word) && word==PeerPrecise[0]);
 assert(!ProgramWord(1,0x2ffff,word) && !ProgramWord(120,0x20000,word));
 assert(CodeAddress(0,0)==0x6000 && CodeAddress(1,1)==0x20000);
 return 0;
}
