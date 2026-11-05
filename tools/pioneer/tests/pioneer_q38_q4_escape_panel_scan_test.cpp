/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4EscapePanelScan.h"
#include "../PioneerQ38Q4EscapeTiles.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#ifdef NDEBUG
#error Assertions required
#endif
using S=PioneerQ38Q4EscapePanelScan;
static unsigned Independent(const std::vector<uint8_t>& rows,unsigned blocks,unsigned block){
 unsigned mask=0;
 for(unsigned n=0;n<16;n++)for(unsigned k=0;k<256;k++){
  const unsigned at=(n*blocks+block)*144+16+(k/64)*32+k%32;
  const unsigned q=(rows[at]>>(k%64>=32?4:0))&15;
  if(q==15)mask|=1U<<(k/16);
 }
 return mask;
}
int main(){
 unsigned cases=0,refusals=0;uint32_t random=0x79a3201U;
 for(unsigned blocks:{20U,24U,68U}){
  std::vector<uint8_t> rows(blocks*16*144,0xee);
  for(unsigned block:{0U,blocks/2,blocks-1}){
   unsigned mask=0xdeadbeef,steps=0;
   assert(S::Mask(rows.data(),unsigned(rows.size()),blocks,block,mask,steps) && !mask && steps==4096);
   for(unsigned k=0;k<256;k++)for(unsigned n=0;n<16;n++){
    const unsigned at=(n*blocks+block)*144+16+(k/64)*32+k%32;
    const unsigned shift=k%64>=32?4:0;rows[at]|=uint8_t(1U<<shift);
    assert(S::Mask(rows.data(),unsigned(rows.size()),blocks,block,mask,steps)
     && mask==(1U<<(k/16)) && mask==Independent(rows,blocks,block) && steps>=16 && steps<=4096);
    rows[at]=0xee;cases++;
   }
   for(unsigned fixture=0;fixture<257;fixture++){
    for(unsigned n=0;n<16;n++)for(unsigned i=0;i<128;i++){
     random^=random<<13;random^=random>>17;random^=random<<5;
     rows[(n*blocks+block)*144+16+i]=fixture==0?0:fixture==1?255:uint8_t(random);
    }
    assert(S::Mask(rows.data(),unsigned(rows.size()),blocks,block,mask,steps) && mask==Independent(rows,blocks,block));
    std::array<uint8_t,2304> tiled{};std::array<uint8_t,2816> encoded{};
    for(unsigned b=0;b<144;b++)for(unsigned n=0;n<16;n++)tiled[b*16+n]=rows[(n*blocks+block)*144+b];
    assert(PioneerQ38Q4EscapeTiles::Encode(tiled.data(),unsigned(tiled.size()),encoded.data(),unsigned(encoded.size())));
    unsigned native=0;for(unsigned p=0;p<16;p++)for(unsigned b=0;b<32;b++)if(encoded[2304+p*32+b])native|=1U<<p;
    assert(mask==native);cases++;
   }
   rows.assign(rows.size(),0xee);
  }
  for(unsigned width:{0U,1U,19U,21U,23U,25U,67U,69U}){
   unsigned mask=0x12345678,steps=0x87654321;
   assert(!S::Mask(rows.data(),unsigned(rows.size()),width,0,mask,steps) && mask==0x12345678 && steps==0x87654321);refusals++;
  }
  for(unsigned size:{0U,unsigned(rows.size())-1,unsigned(rows.size())+1}){
   unsigned mask=1,steps=2;assert(!S::Mask(rows.data(),size,blocks,0,mask,steps) && mask==1 && steps==2);refusals++;
  }
  unsigned mask=1,steps=2;
  assert(!S::Mask(nullptr,unsigned(rows.size()),blocks,0,mask,steps) && mask==1 && steps==2);refusals++;
  assert(!S::Mask(rows.data(),unsigned(rows.size()),blocks,blocks,mask,steps) && mask==1 && steps==2);refusals++;
 }
 printf("PASS_ESCAPE_PANEL_DATA_SCAN cases%u inertRefusals%u everyK16N16position_all3widths_firstMiddleLastBlock independentBitLoop_and_actualNativeCodec NO_MODEL_DEVICE_OR_SPEED_PROOF\n",cases,refusals);
}
