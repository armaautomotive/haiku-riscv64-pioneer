/* SPDX-License-Identifier: MIT */
// Off-card check against the GGUF Q5_K row layout and scalar reconstruction.
#include "../PioneerQwen35Q5Panel.h"
#include "../PioneerAttentionMath.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <random>

static float Float(uint32_t bits) { float value; memcpy(&value,&bits,4); return value; }
static int Operand(uint16_t bits) {
 assert(PioneerInt8OperandValid(bits));
 return int(bits&1023)*((bits&0x8000)?-1:1);
}

// Reference follows the original GGUF row organization, not the transposed
// output indexing. Scales 0..3 and 4..7 have different bit encodings.
static void Reference(const uint8_t* b,unsigned i,int& q,float& coefficient,float& minimum) {
 const unsigned group=i/32;
 const uint8_t* s=b+4;
 unsigned sc=group<4?s[group]&63:(s[group+4]&15)|((s[group-4]>>6)<<4);
 unsigned mn=group<4?s[group+4]&63:(s[group+4]>>4)|((s[group]>>6)<<4);
 unsigned lo=b[48+(group/2)*32+i%32];
 q=int(((group&1)?lo>>4:lo&15)+((b[16+i%32]&(1U<<group))?16:0));
 coefficient=PioneerAttentionHalf(uint16_t(b[0]|(b[1]<<8)))*float(sc);
 minimum=PioneerAttentionHalf(uint16_t(b[2]|(b[3]<<8)))*float(-int(mn));
}

using Rows=std::array<uint8_t,16*176>;
static void Check(const Rows& rows) {
  alignas(4) Rows tile{};
  for (unsigned r=0;r<16;r++)
   for (unsigned b=0;b<176;b++) tile[b*16+r]=rows[r*176+b];
  for (unsigned p=0;p<16;p++) {
   PioneerQ38Panel result{}; assert(PioneerQwen35Q5Panel(tile.data(),tile.size(),p,&result));
   for (unsigned r=0;r<16;r++) for (unsigned lane=0;lane<16;lane++) {
    int q; float c,m; Reference(rows.data()+r*176,p*16+lane,q,c,m);
    unsigned n=lane*16+r;
    assert(Operand(uint16_t(result.operand[n/2]>>((n&1)*16)))==q);
    assert(Float(result.coefficient[r])==c && Float(result.minimum[r])==m);
    assert(Float(result.coefficient[r])*float(q)+Float(result.minimum[r])==c*float(q)+m);
   }
  }
  PioneerQ38Panel result; memset(&result,0xa5,sizeof(result)); auto before=result;
  assert(!PioneerQwen35Q5Panel(tile.data(),tile.size()-1,0,&result));
  assert(!PioneerQwen35Q5Panel(tile.data(),tile.size(),16,&result));
  assert(!PioneerQwen35Q5Panel(tile.data(),tile.size(),0,reinterpret_cast<PioneerQ38Panel*>(tile.data())));
  // Failure in the final row must leave all output untouched.
  tile[16+15]=0x7c;
  assert(!PioneerQwen35Q5Panel(tile.data(),tile.size(),0,&result));
  assert(!memcmp(&result,&before,sizeof(result)));
}

int main(int argc,char** argv) {
 if (argc==2 && !strcmp(argv[1],"--stdin")) {
  Rows rows{}; unsigned count=0;
  while (size_t n=fread(rows.data(),1,rows.size(),stdin)) {
   assert(n==rows.size()); Check(rows); count++;
  }
  assert(!ferror(stdin) && count==432);
  printf("PASS: %u authenticated real Q5_K tiles, all panels/rows/lanes; off-card only\n",count);
  return 0;
 }
 assert(argc==1);
 std::mt19937 random(0x354b);
 for (unsigned trial=0;trial<512;trial++) {
  Rows rows{};
  for (unsigned r=0;r<16;r++) {
   for (unsigned b=0;b<176;b++) rows[r*176+b]=uint8_t(random());
   for (unsigned b:{0U,2U}) {
    uint16_t h=uint16_t(random()); if ((h&0x7c00)==0x7c00) h^=0x400;
    rows[r*176+b]=uint8_t(h); rows[r*176+b+1]=uint8_t(h>>8);
   }
  }
  Check(rows);
 }
 puts("PASS: 512 synthetic Q5_K tiles, all 16 panels/rows/lanes, exact quant codes and affine coefficients; failure atomicity; off-card only");
}
