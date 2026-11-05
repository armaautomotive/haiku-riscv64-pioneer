/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_ESCAPE_TILES_H
#define PIONEER_Q38_Q4_ESCAPE_TILES_H
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Exact Q4 source-format prerequisite only,not a shader/native/Work or CPU inference path
#endif
#include <array>
#include <cstdint>
#include <cstring>

// Lossless integer factorization,NOT dequantized-weight requantization:
// q = z + 7 + escape, z=min(q,14)-7 in[-7,7],escape=(q==15).
// Native BFP4_B magnitude3/sign1 with shared exponent129 represents z
// EXACTLY. The additional bit distinguishes q14/q15. Original d,dmin and
// six-bit factors stay byte-exact;no HALF/FP32 products occur here.
// A future kernel must reconstruct the INTEGER dot BEFORE the original
// affine arithmetic. Folding7 into the minimum would change FP32 rounding.
// Native UNPACK forced-exponent mode can omit an exponent stream,subject
// to its documented16-byte alignment/burst constraints. This file does NOT
// implement/prove any unpack,matrix,bank,source,lifecycle or speed behavior.
struct PioneerQ38Q4EscapeTiles {
 static constexpr unsigned OriginalBytes=2304,MetadataBytes=256,PrimaryBytes=2304,
  MaskBytes=512,DenseBytes=2816,Elements=4096,Rows=256;
 enum MaskKind:unsigned {Dense=0,SparseRows=1,Zero=2};
 struct Mask {unsigned kind=Zero,bytes=0,nonzeroRows=0;};
 static bool Separate(const void* a,unsigned n,const void* b,unsigned m) {
  const auto x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b),limit=~uintptr_t(0);
  return a && b && x<=limit-n && y<=limit-m && (x+n<=y || y+m<=x);
 }
 static unsigned OriginalAt(unsigned panel,unsigned i) {
  return MetadataBytes+(panel/4)*512+(panel%2)*256+i;
 }
 static bool Encode(const uint8_t* original,unsigned bytes,uint8_t* output,unsigned capacity) {
  if(bytes!=OriginalBytes || capacity<DenseBytes || !Separate(original,bytes,output,DenseBytes))return false;
  std::array<uint8_t,DenseBytes> stage{};std::memcpy(stage.data(),original,MetadataBytes);
  for(unsigned p=0;p<16;p++)for(unsigned i=0;i<256;i++) {
   unsigned q=(original[OriginalAt(p,i)]>>(p&2?4:0))&15;
   const int z=int(q==15?14:q)-7;
   const unsigned sm=z<0?unsigned(-z)|8:unsigned(z);
   stage[MetadataBytes+p*128+i/2]|=uint8_t(sm<<((i&1)*4));
   stage[PrimaryBytes+p*32+i/8]|=uint8_t(unsigned(q==15)<<(i&7));
  }
  std::memcpy(output,stage.data(),DenseBytes);return true;
 }
 static bool Decode(const uint8_t* encoded,unsigned bytes,uint8_t* original,unsigned capacity) {
  if(bytes!=DenseBytes || capacity<OriginalBytes || !Separate(encoded,bytes,original,OriginalBytes))return false;
  std::array<uint8_t,OriginalBytes> stage{};std::memcpy(stage.data(),encoded,MetadataBytes);
  for(unsigned p=0;p<16;p++)for(unsigned i=0;i<256;i++) {
   unsigned sm=(encoded[MetadataBytes+p*128+i/2]>>((i&1)*4))&15;
   unsigned escape=(encoded[PrimaryBytes+p*32+i/8]>>(i&7))&1;
   if(sm==8 || (escape && sm!=7))return false; // no negative zero/noncanonical pair
   const int z=(sm&8)?-int(sm&7):int(sm&7);
   unsigned q=unsigned(z+7+int(escape));
   stage[OriginalAt(p,i)]|=uint8_t(q<<(p&2?4:0));
  }
  std::memcpy(original,stage.data(),OriginalBytes);return true;
 }
 static unsigned NonzeroRows(const uint8_t* dense) {
  unsigned count=0;for(unsigned r=0;r<Rows;r++)count+=bool(dense[2*r]|dense[2*r+1]);return count;
 }
 static unsigned MaskPayloadBytes(unsigned count) {
  return count>Rows?~0U:!count?0:32+2*count<MaskBytes?32+2*count:MaskBytes;
 }
 static bool PackMask(const uint8_t* dense,unsigned bytes,uint8_t* output,unsigned capacity,Mask& result) {
  if(bytes!=MaskBytes || capacity<MaskBytes || !Separate(dense,bytes,output,MaskBytes)
   || !Separate(&result,sizeof(result),dense,bytes) || !Separate(&result,sizeof(result),output,MaskBytes))return false;
  const unsigned count=NonzeroRows(dense);Mask next;next.nonzeroRows=count;
  std::array<uint8_t,MaskBytes> stage{};
  if(!count){result=next;return true;} // no payload,output untouched
  if(32+2*count<MaskBytes) {
   next.kind=SparseRows;next.bytes=32+2*count;unsigned at=32;
   for(unsigned r=0;r<Rows;r++)if(dense[2*r]|dense[2*r+1]) {
    stage[r/8]|=uint8_t(1U<<(r&7));stage[at++]=dense[2*r];stage[at++]=dense[2*r+1];
   }
  } else {next.kind=Dense;next.bytes=MaskBytes;std::memcpy(stage.data(),dense,MaskBytes);}
  std::memcpy(output,stage.data(),next.bytes);result=next;return true;
 }
 static bool UnpackMask(const uint8_t* input,unsigned bytes,const Mask& description,uint8_t* dense,unsigned capacity) {
  if(capacity<MaskBytes || bytes!=description.bytes || bytes>MaskBytes || description.nonzeroRows>Rows
   || !Separate(input,bytes,dense,MaskBytes) || !Separate(&description,sizeof(description),dense,MaskBytes))return false;
  std::array<uint8_t,MaskBytes> stage{};
  if(description.kind==Zero) {if(bytes || description.nonzeroRows)return false;}
  else if(description.kind==Dense) {
   if(bytes!=MaskBytes || !description.nonzeroRows || 32+2*description.nonzeroRows<MaskBytes)return false;
   std::memcpy(stage.data(),input,MaskBytes);
  } else if(description.kind==SparseRows) {
   if(!description.nonzeroRows || bytes!=32+2*description.nonzeroRows || bytes>=MaskBytes)return false;
   unsigned at=32;
   for(unsigned r=0;r<Rows;r++)if(input[r/8]&(1U<<(r&7))) {
    if(at>bytes-2 || !(input[at]|input[at+1]))return false;
    stage[2*r]=input[at++];stage[2*r+1]=input[at++];
   }
   if(at!=bytes)return false;
  } else return false;
  if(NonzeroRows(stage.data())!=description.nonzeroRows)return false;
  std::memcpy(dense,stage.data(),MaskBytes);return true;
 }
};
#endif
