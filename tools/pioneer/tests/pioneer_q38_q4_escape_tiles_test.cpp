/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4EscapeTiles.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Exact source tests must not be compiled out
#endif
using E=PioneerQ38Q4EscapeTiles;
int main() {
 unsigned cases=0,dots=0;
 for(unsigned seed=0;seed<256;seed++) {
  std::array<uint8_t,2304> original{},decoded{};std::array<uint8_t,2817> packed{};
  for(unsigned i=0;i<256;i++)original[i]=uint8_t(seed+i*17);
  for(unsigned k=0;k<256;k++)for(unsigned n=0;n<16;n++) {
   unsigned q=(seed+k*17+n*3)&15,group=k/32;
   unsigned at=(16+(group/2)*32+k%32)*16+n;
   original[at]|=uint8_t(q<<((group&1)*4));
  }
  packed.fill(0xa5);assert(E::Encode(original.data(),original.size(),packed.data(),2816) && packed.back()==0xa5);
  assert(!memcmp(original.data(),packed.data(),256));
  assert(E::Decode(packed.data(),2816,decoded.data(),decoded.size()) && original==decoded);
  for(unsigned p=0;p<16;p++)for(unsigned n=0;n<16;n++) {
   int originalDot=0,nativeDot=0,correction=0,sum=0;
   for(unsigned k=0;k<16;k++) {
    unsigned i=k*16+n,sm=(packed[256+p*128+i/2]>>((i&1)*4))&15;
    unsigned bit=(packed[2304+p*32+i/8]>>(i&7))&1;
    int z=(sm&8)?-int(sm&7):int(sm&7),activation=int((seed*7+k*13)&255)-128;
    unsigned q=(seed+(p*16+k)*17+n*3)&15;
    assert(sm!=8 && z>=-7 && z<=7 && (!bit || z==7) && z+7+int(bit)==int(q));
    originalDot+=int(q)*activation;nativeDot+=z*activation;correction+=int(bit)*activation;sum+=activation;
   }
   assert(originalDot==nativeDot+correction+7*sum);dots++;
  }
  std::array<uint8_t,512> mask{},restored{};E::Mask description;
  assert(E::PackMask(packed.data()+2304,512,mask.data(),512,description));
  assert(E::UnpackMask(mask.data(),description.bytes,description,restored.data(),512));
  assert(!memcmp(restored.data(),packed.data()+2304,512));cases++;
 }
 // Every sparse/dense transition and the all-zero no-payload case.
 for(unsigned count=0;count<=256;count++) {
  std::array<uint8_t,512> raw{},packed{},decoded{};packed.fill(0xa5);E::Mask d;
  for(unsigned r=0;r<count;r++){raw[2*r]=uint8_t(r+1);raw[2*r+1]=1;}
  assert(E::PackMask(raw.data(),512,packed.data(),512,d) && d.nonzeroRows==count);
  assert(d.bytes==E::MaskPayloadBytes(count));
  assert(d.kind==(count==0?E::Zero:32+2*count<512?E::SparseRows:E::Dense));
  assert(E::UnpackMask(packed.data(),d.bytes,d,decoded.data(),512) && raw==decoded);
  if(!count)for(auto v:packed)assert(v==0xa5);
  else for(unsigned i=d.bytes;i<512;i++)assert(packed[i]==0xa5);
 }
 unsigned refused=0;
 std::array<uint8_t,2304> raw{},restored{};std::array<uint8_t,2816> encoded{};
 assert(E::Encode(raw.data(),2304,encoded.data(),2816));
 for(unsigned invalid:{8U,6U}) {
  auto bad=encoded;bad[256]=uint8_t((bad[256]&0xf0)|invalid);if(invalid==6)bad[2304]|=1;
  restored.fill(0xa5);auto old=restored;assert(!E::Decode(bad.data(),2816,restored.data(),2304) && restored==old);refused++;
 }
 assert(!E::Encode(raw.data(),2303,encoded.data(),2816));refused++;
 assert(!E::Decode(encoded.data(),2815,restored.data(),2304));refused++;
 assert(!E::Encode(encoded.data(),2304,encoded.data(),2816));refused++;
 std::array<uint8_t,513> mask{},out{};E::Mask description;
 mask[0]=1;assert(E::PackMask(mask.data(),512,out.data(),512,description));
 assert(description.kind==E::SparseRows && description.bytes==34);
 auto packedMask=out;
 for(unsigned mutation=0;mutation<8;mutation++) {
  auto bad=packedMask;auto d=description;
  if(mutation==0)d.kind=3;
  if(mutation==1)d.nonzeroRows=2;
  if(mutation==2)d.bytes=33;
  if(mutation==3)bad[32]=0; // presence bitmap claiming an all-zero row
  if(mutation==4)bad[1]|=1; // second row exceeds payload
  if(mutation==5)d.kind=E::Dense;
  if(mutation==6)d.kind=E::Zero;
  if(mutation==7)d.nonzeroRows=257;
  out.fill(0xa5);const auto old=out;
  assert(!E::UnpackMask(bad.data(),d.bytes,d,out.data(),512) && out==old);refused++;
 }
 assert(E::MaskPayloadBytes(257)==~0U);
 out.fill(0xa5);const auto old=out;const auto oldDescription=description;
 assert(!E::PackMask(mask.data(),511,out.data(),512,description) && out==old
  && !memcmp(&oldDescription,&description,sizeof(description)));refused++;
 assert(!E::PackMask(mask.data(),512,out.data(),511,description) && out==old);refused++;
 assert(!E::UnpackMask(packedMask.data(),34,description,out.data(),511) && out==old);refused++;
 assert(!E::UnpackMask(packedMask.data(),34,description,packedMask.data(),512));refused++;
 assert(!E::PackMask(mask.data(),512,mask.data(),512,description));refused++;
 printf("PASS_EXACT_NATIVE_Q4_SOURCE cases%u integerDots%u maskTransitions257 refusals%u;NO_WEIGHT_REQUANTIZATION_OR_HARDWARE\n",cases,dots,refused);
}
