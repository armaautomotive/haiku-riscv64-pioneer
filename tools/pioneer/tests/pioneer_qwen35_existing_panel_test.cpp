/* SPDX-License-Identifier: MIT */
// Offline comparison of existing Q4_K/Q6_K/Q8_0 panels with scalar GGUF rows.
#include "../PioneerQwen38Panel.h"
#include "../TenstorrentGgufQuant.h"
#include "../PioneerAttentionMath.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>

int main(int argc,char** argv) {
 assert(argc==2);
 unsigned type=unsigned(atoi(argv[1]));
 unsigned bytes=type==8?34:type==12?144:type==14?210:0;
 unsigned tileBytes=PioneerQ38PanelTileBytes(type),panels=type==8?2:16;
 assert(bytes && tileBytes);
 std::array<uint8_t,16*210> raw{};
 alignas(4) std::array<uint8_t,3392> tile{};
 unsigned cases=0;
 while (size_t n=fread(raw.data(),1,bytes*16,stdin)) {
  assert(n==bytes*16); tile.fill(0);
  for (unsigned row=0;row<16;row++) for (unsigned b=0;b<bytes;b++)
   tile[b*16+row]=raw[row*bytes+b];
  float expected[16][256]{};
  for (unsigned row=0;row<16;row++) {
   const uint8_t* source=raw.data()+row*bytes;
   assert(type==8?PioneerDecodeQ8_0(source,expected[row]):type==12?
    PioneerDecodeQ4K(source,expected[row]):PioneerDecodeQ6K(source,expected[row]));
  }
  for (unsigned p=0;p<panels;p++) {
   PioneerQ38Panel out{}; assert(PioneerQ38UnpackPanel(type,tile.data(),tileBytes,p,&out));
   for (unsigned lane=0;lane<16;lane++) for (unsigned row=0;row<16;row++) {
    unsigned at=lane*16+row; uint16_t bits=uint16_t(out.operand[at/2]>>((at&1)*16));
    assert(PioneerInt8OperandValid(bits));
    int q=int(bits&1023)*((bits&0x8000)?-1:1);
    float result=PioneerAttentionFloat(out.coefficient[row])*q+PioneerAttentionFloat(out.minimum[row]);
    assert(result==expected[row][p*16+lane]);
   }
  }
  cases++;
 }
 assert(!ferror(stdin) && cases);
 printf("PASS: %u real type %u tiles against scalar GGUF rows; off-card only\n",cases,type);
}
