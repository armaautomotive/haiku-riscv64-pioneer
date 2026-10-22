/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PREDECODED_Q4_PAGE_STUDY
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
#include "../PioneerQ38PredecodedQ4Reader.h"
struct Tensor {uint64_t inputs,outputs,bytes;unsigned type=12,rank=2;};
static uint8_t PackedByte(unsigned tensor,uint64_t at,unsigned pattern) {
 uint32_t word=uint32_t(at)*0x45d9f3bU+uint32_t(at>>32)*0x85ebca6bU+(tensor+1)*0x9e3779b9U+pattern*0x384732U;
 word^=word>>16;word*=0x85ebca6bU;word^=word>>13;return uint8_t(word);
}
struct Model {
 std::vector<Tensor> tensors;
 uint64_t epoch=1;
 unsigned pattern=0,reads=0,failRead=0,changeEpochRead=0,loseStableRead=0;
 bool stable=true;
 explicit Model(unsigned k=5120){tensors.push_back({k,512,uint64_t(k/256)*144*512});}
 bool Stable()const{return stable;}
 uint64_t Epoch()const{return epoch;}
 const std::vector<Tensor>& Tensors()const{return tensors;}
 bool ReadPacked(unsigned tensor,uint64_t relative,unsigned bytes,void* destination,unsigned capacity) {
  assert(tensor<tensors.size() && destination && bytes && bytes<=capacity && capacity==156672);
  const auto& t=tensors[tensor];assert(relative<=t.bytes && bytes<=t.bytes-relative);
  assert(bytes==16*(t.inputs/256)*144 && relative%bytes==0);reads++;
  auto* out=static_cast<uint8_t*>(destination);
  for(unsigned b=0;b<bytes;b++)out[b]=PackedByte(tensor,relative+b,pattern);
  if(reads==changeEpochRead)epoch++;
  if(reads==loseStableRead)stable=false;
  return reads!=failRead;
 }
};
using Reader=PioneerQ38PredecodedQ4Reader<Model>;
static uint8_t Expected(unsigned tensor,unsigned k,unsigned firstRow,uint64_t at,unsigned pattern) {
 unsigned blocks=k/256,tile=unsigned(at/4352),within=unsigned(at%4352);
 unsigned byte=within,shift=0;
 if(within>=256) {
  unsigned panel=(within-256)/256,lane=(within-256)%256;
  byte=256+(panel/4)*512+(panel%2)*256+lane;shift=panel%4<2?0:4;
 }
 unsigned row=firstRow+(tile/blocks)*16+byte%16,block=tile%blocks;
 uint64_t raw=uint64_t(row)*blocks*144+block*144+byte/16;
 uint8_t value=PackedByte(tensor,raw,pattern);
 return within<256?value:uint8_t((value>>shift)&15);
}
int main() {
 std::array<uint8_t,4098> out;unsigned pages=0,fragments=0;
 for(unsigned kind=0;kind<3;kind++) {
  Model source;Reader candidate;out.fill(0xdd);
  assert(!candidate.ReadPage(source,0,0,16,0,out.data()+1,4096) && !source.reads);
  if(kind==0)source.stable=false;if(kind==1)source.epoch=0;
  if(kind<2) {
   assert(!candidate.Bind(source));source.stable=true;source.epoch=1;
   assert(!candidate.Bind(source) && !candidate.ReadPage(source,0,0,16,0,out.data()+1,4096));
   for(auto b:out)assert(b==0xdd);assert(!source.reads);
  } else assert(candidate.Bind(source) && !candidate.Bind(source));
 }
 for(unsigned k:{5120U,6144U,17408U})for(unsigned pattern:{0U,1U}) {
  Model model(k);model.pattern=pattern;Reader reader;assert(reader.Bind(model));
  for(unsigned first:{0U,16U,32U})for(unsigned rows:{16U,48U,256U}) {
   uint32_t bytes=0;assert(reader.FragmentBytes(model,0,first,rows,bytes));
   assert(bytes==uint64_t(rows/16)*(k/256)*4352);fragments++;
   for(uint64_t page=0;page<((uint64_t(bytes)+4095)/4096);page++) {
    out.fill(0xa5);assert(reader.ReadPage(model,0,first,rows,page,out.data()+1,4096));
    assert(out.front()==0xa5 && out.back()==0xa5);
    for(unsigned b=0;b<4096;b++) {
     uint64_t at=page*4096+b;
     assert(out[1+b]==(at<bytes?Expected(0,k,first,at,pattern):0));
    }
    pages++;
   }
  }
 }
 // Cache reuse is tied to exact source/tensor/row/epoch,not merely a page.
 Model model;model.tensors.push_back(model.tensors[0]);Reader reader;assert(reader.Bind(model));
 assert(reader.ReadPage(model,0,0,16,0,out.data()+1,4096));unsigned reads=model.reads;
 assert(reader.ReadPage(model,0,0,16,0,out.data()+1,4096) && model.reads==reads);
 for(unsigned tensor:{1U,0U}) {
  assert(reader.ReadPage(model,tensor,0,16,0,out.data()+1,4096));
  for(unsigned b=0;b<4096;b++)assert(out[1+b]==Expected(tensor,5120,0,b,0));
 }
 assert(model.reads==reads+2);
 // Page21 crosses TWO sixteen-row source-cache groups for K5120.
 unsigned failures=0;
 for(unsigned kind=0;kind<3;kind++)for(unsigned call:{1U,2U}) {
  Model broken;Reader candidate;assert(candidate.Bind(broken));
  if(kind==0)broken.failRead=call;if(kind==1)broken.changeEpochRead=call;if(kind==2)broken.loseStableRead=call;
  out.fill(0xdd);assert(!candidate.ReadPage(broken,0,0,48,21,out.data()+1,4096));
  for(auto b:out)assert(b==0xdd);
  unsigned oldReads=broken.reads;broken.stable=true;
  assert(!candidate.ReadPage(broken,0,0,16,0,out.data()+1,4096) && broken.reads==oldReads);
  assert(!candidate.Bind(broken));failures++;
 }
 // Invalidate an already populated cache without a second source read.
 for(unsigned kind=0;kind<2;kind++) {
  Model broken;Reader candidate;assert(candidate.Bind(broken));
  assert(candidate.ReadPage(broken,0,0,16,0,out.data()+1,4096));
  if(kind)broken.epoch++;else broken.stable=false;
  out.fill(0xdd);unsigned oldReads=broken.reads;
  assert(!candidate.ReadPage(broken,0,0,16,0,out.data()+1,4096));
  for(auto b:out)assert(b==0xdd);assert(broken.reads==oldReads);failures++;
 }
 for(unsigned fault=0;fault<15;fault++) {
  Model source;Reader candidate;assert(candidate.Bind(source));Model other;
  unsigned tensor=0,first=0,rows=16,capacity=4096;uint64_t page=0;
  uint8_t* output=out.data()+1;out.fill(0xdd);Model* selected=&source;
  if(fault==0)tensor=1;if(fault==1)first=1;if(fault==2)rows=0;if(fault==3)rows=17;
  if(fault==4)first=512;if(fault==5)rows=528;if(fault==6)page=UINT64_MAX;
  if(fault==7)source.tensors[0].type=8;if(fault==8)source.tensors[0].rank=1;
  if(fault==9)source.tensors[0].inputs=4096;if(fault==10)source.tensors[0].bytes--;
  if(fault==11)source.tensors[0].outputs=UINT64_MAX;if(fault==12)capacity=4095;
  if(fault==13)output=nullptr;if(fault==14)selected=&other;
  assert(!candidate.ReadPage(*selected,tensor,first,rows,page,output,capacity));
  for(auto b:out)assert(b==0xdd);assert(!source.reads && !other.reads);failures++;
 }
 // Fragment size overflow is refused atomically BEFORE packed data reads.
 Model enormous(17408);enormous.tensors[0].outputs=UINT32_MAX-15;
 enormous.tensors[0].bytes=enormous.tensors[0].outputs*(17408/256)*144;
 Reader candidate;assert(candidate.Bind(enormous));uint32_t bytes=0xdeadbeef;
 assert(!candidate.FragmentBytes(enormous,0,0,UINT32_MAX-15,bytes) && bytes==0xdeadbeef && !enormous.reads);failures++;
 printf("PASS OFFLINE Q4 row-to-page DATA fragments%u pages%u refusals%u;all3widths/twochangedpatterns/allbytes/metadata/padding/source-tensor-row-cache/freshness/lateatomicfailure/unbound-unstable-zeroepoch-rebind guards;NO_AUTH_SOURCE_LAYOUT_DEVICE_OR_NATIVE_AUTHORITY\n",fragments,pages,failures);
}
