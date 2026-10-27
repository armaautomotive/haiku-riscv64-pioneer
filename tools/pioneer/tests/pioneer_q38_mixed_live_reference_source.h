/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_REFERENCE_SOURCE_H
#define PIONEER_Q38_MIXED_LIVE_REFERENCE_SOURCE_H
#include <cassert>
#include <string>
#include <vector>
// ORIGINAL ROW-major synthetic byte source. No reference products, card
// transport, native scope, private producer or prepared lease is provided.
struct MixedReferenceSource {
 struct Tensor {std::string name;unsigned type,rank,inputs,outputs;uint64_t bytes,offset;};
 struct Index {
  std::vector<Tensor> tensors;uint64_t epoch=1;
  const auto& Tensors()const{return tensors;}uint64_t Epoch()const{return epoch;}
 } model;
 bool valid=true,zero=false;unsigned reads=0,failRead=0,mutateRead=0,mutateSlot=11,roundRead=0,dropRead=0,epochRead=0,reallocateRead=0,badRead=0;
 MixedReferenceSource(){
  const char* names[]={"token_embd.weight","blk.3.attn_norm.weight","blk.3.post_attention_norm.weight","blk.3.attn_q.weight",
   "blk.3.attn_k.weight","blk.3.attn_v.weight","blk.3.attn_q_norm.weight","blk.3.attn_k_norm.weight","blk.3.attn_output.weight",
   "blk.3.ffn_gate.weight","blk.3.ffn_up.weight","blk.3.ffn_down.weight"};
  const unsigned type[]={12,0,0,8,8,8,0,0,14,12,12,12},inputs[]={5120,5120,5120,5120,5120,5120,256,256,6144,5120,5120,17408};
  const unsigned rows[]={248320,1,1,12288,1024,1024,1,1,5120,17408,17408,5120};
  for(unsigned i=0;i<12;i++){
   unsigned block=type[i]==8?32:type[i]?256:1,bytes=type[i]==8?34:type[i]==12?144:type[i]==14?210:4;
   model.tensors.push_back({names[i],type[i],type[i]?2U:1U,inputs[i],rows[i],uint64_t(inputs[i]/block)*bytes*rows[i],1000+i*100});
  }
  for(unsigned i=12;i<851;i++)model.tensors.push_back({"unused_"+std::to_string(i),0,1,1,1,4,100000+i*4});
 }
 bool Ready()const{return valid;}const auto& Model()const{return model;}
 static unsigned Scale(unsigned row,unsigned block,unsigned group){return (row+block+group*11)%64;}
 static unsigned Minimum(unsigned row,unsigned block,unsigned group){return (row*7+block+group*3)%64;}
 static unsigned Q4(unsigned row,unsigned block,unsigned lane){return (row*7+block*3+lane*5)%16;}
 static int Q8(unsigned row,unsigned block,unsigned lane){return int((row*3+block*11+lane*17)%256)-128;}
 static unsigned Q6(unsigned row,unsigned block,unsigned lane){return (row*7+block*13+lane*17)%64;}
 static int Scale6(unsigned row,unsigned block,unsigned panel){return int((row*5+block*11+panel*7)%256)-128;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* output,unsigned capacity){
  assert(tensor<12 && output && bytes==capacity && offset<=model.tensors[tensor].bytes && bytes<=model.tensors[tensor].bytes-offset);
  if(++reads==failRead)return false;auto* p=static_cast<uint8_t*>(output);std::memset(p,0,bytes);const auto& t=model.tensors[tensor];
  if(!t.type){assert(!offset && bytes==t.inputs*4);for(unsigned i=0;i<t.inputs;i++){p[i*4+2]=0x80;p[i*4+3]=0x3f;}}
  else{
   unsigned width=t.type==8?32:256,packed=t.type==8?34:t.type==12?144:210,blocks=t.inputs/width,rowBytes=blocks*packed;
   assert(!(offset%rowBytes) && !(bytes%rowBytes));unsigned rows=bytes/rowBytes,first=unsigned(offset/rowBytes);
   for(unsigned row=0;row<rows;row++)for(unsigned b=0;b<blocks;b++){
    auto* block=p+row*rowBytes+b*packed;unsigned r=first+row;
    if(t.type==8){if(!zero)block[1]=0x10;for(unsigned i=0;i<32;i++)block[2+i]=uint8_t(Q8(r,b,i));}
    else if(t.type==12){
     if(!zero || !tensor){block[0]=tensor?0x10:0;block[1]=tensor?0:0x28;block[2]=tensor?8:0;block[3]=tensor?0:0x24;}
     unsigned d[8],m[8];for(unsigned g=0;g<8;g++){d[g]=Scale(r,b,g);m[g]=Minimum(r,b,g);}
     for(unsigned g=0;g<4;g++){block[4+g]=uint8_t(d[g]|((d[g+4]>>4)<<6));block[8+g]=uint8_t(m[g]|((m[g+4]>>4)<<6));block[12+g]=uint8_t((d[g+4]&15)|((m[g+4]&15)<<4));}
     for(unsigned g=0;g<4;g++)for(unsigned i=0;i<32;i++)block[16+g*32+i]=uint8_t(Q4(r,b,g*64+i)|(Q4(r,b,g*64+32+i)<<4));
    }else{
     assert(t.type==14);if(!zero)block[208]=0x10;
     for(unsigned i=0;i<256;i++){
      unsigned h=i/128,g=i%128/32,l=i%32,q=Q6(r,b,i);
      block[h*64+(g%2)*32+l]|=uint8_t((q&15)<<((g/2)*4));block[128+h*32+l]|=uint8_t((q>>4)<<(g*2));
     }
     for(unsigned g=0;g<16;g++)block[192+g]=uint8_t(Scale6(r,b,g));
    }
   }
  }
  if(reads==badRead){p[0]=0;p[1]=0x7f;p[2]=0xc0;p[3]=0x7f;}
  if(reads==mutateRead)model.tensors[mutateSlot].offset++;
  if(reads==dropRead)valid=false;if(reads==epochRead)model.epoch++;
  if(reads==reallocateRead){auto replacement=model.tensors;model.tensors.swap(replacement);}
  if(reads==roundRead)assert(!std::fesetround(FE_DOWNWARD));return true;
 }
};
#endif
