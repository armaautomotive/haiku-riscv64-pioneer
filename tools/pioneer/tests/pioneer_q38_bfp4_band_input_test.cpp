/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Bfp4BandInput.h"
#include <cassert>
#include <cstdio>
int main(){
 unsigned different=0;
 // Independently follow the original Q4 exporter's nested block/column loop.
 for(unsigned block=0;block<20;block++)for(unsigned c=0;c<256;c++){
  unsigned i=block*256+c;
  uint16_t q4=uint16_t(0x3e80+(c*19+block*11)%384+((c&3)==2?0x8000:0));
  uint16_t legacy=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
  assert(PioneerQ38Bfp4BandInput::Word(i,true)==q4);
  assert(PioneerQ38Bfp4BandInput::Word(i,false)==legacy);
  different+=q4!=legacy;
 }
 assert(different>0);
 printf("PASS matched Q4 and unchanged legacy input/all5120 BF16 words/different%u; NO MODEL OR DEVICE\n",different);
}
