/* SPDX-License-Identifier: MIT */
// One actual uninterrupted ROOT invocation through the complete layer and
// its next-input link. ALL119 actual peer programs/sequence/counter/tensor
// states are retained; the observer supplies NO intermediate numeric data.
#define PIONEER_Q38_RESIDENT_LAYER_MACHINE_ONLY
#include "pioneer_q38_resident_layer_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerLinkPlan.h"
struct LinkedLayerFabric:LayerFabric {
 std::vector<uint8_t> linkCode;
 PioneerQ38ResidentLayerLinkPlan link;
 std::array<uint32_t,5120> finalHidden={};std::array<uint32_t,8> linkBefore={};
 std::array<unsigned,119> peerSequence={};
 unsigned linkEntry=0,linkCaptures=0,linkPackets=0,linkNormStores=0;
 bool entered=false;
 LinkedLayerFabric(const char* image,const char* peer,const OutputParts& geometry):LayerFabric(image,peer,geometry){
  linkCode=Read((std::string(image)+"/link.bin").c_str());assert(!linkCode.empty() && linkCode.size()<=0x2a00);
  linkEntry=Symbol(image,"q38_resident_layer_link_once");
  assert(link.Build(*canonical,*plan));
  for(unsigned i=0;i<128;i++)Mem(TT_Q38_LAYER_LINK_MANIFEST+i*4)=link.Descriptor()[i];
  const auto* n=link.Descriptor()+64;
  for(unsigned i=0;i<5120;i++)Put(n[12],n[11]+i*4,Bits(.625f+float((i*37)%127)/256.f));
 }
 bool LinkCode(unsigned at)const{return Range(at,TT_Q38_LAYER_LINK_ROM,unsigned(linkCode.size()));}
 bool CodeAddress(uint32_t at)const override{return LinkCode(at) || LayerFabric::CodeAddress(at);}
 unsigned Index(uint32_t at)override{if(LinkCode(at)){l1[8177]=Word(linkCode,at-TT_Q38_LAYER_LINK_ROM);return 8177;}return LayerFabric::Index(at);}
 bool RangeData(unsigned at)const override{return LayerFabric::RangeData(at) || Range(at,TT_Q38_LAYER_LINK_MANIFEST,1408);}
 uint32_t Load(uint32_t at)override{return LinkCode(at)?Word(linkCode,at-TT_Q38_LAYER_LINK_ROM):LayerFabric::Load(at);}
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,TT_Q38_LAYER_LINK_SHADOW,512)){
   assert(!packets && at==TT_Q38_LAYER_LINK_SHADOW+linkCaptures*4 && value==Mem(TT_Q38_LAYER_LINK_MANIFEST+linkCaptures*4));
   linkCaptures++;Mem(at)=value;return;
  }
  if(Range(at,TT_Q38_LAYER_LINK_RECEIPT,384)){assert(!unknown && !pending);Mem(at)=value;return;}
  if(at==0x17ffc8 && value==TT_Q38_LAYER_LINK_FAULT){unknown=true;Mem(at)=value;return;}
  if(entered && Range(at,0x90000,20480))linkNormStores++;
  LayerFabric::Store(at,value);
 }
 void Submit()override{
  if(!entered){LayerFabric::Submit();return;}
  unsigned p=linkPackets++;packets++;assert(p<8);
  bool write=p>=4,learned=p>=2 && p<4,normalizedWrite=p>=6;
  const auto* n=link.Descriptor()+64;
  unsigned offset=(p&1)*16384,bytes=p&1?4096:16384;
  unsigned local=(write?(normalizedWrite?0x90000:0x80000):learned?0x88000:0x80000)+offset;
  unsigned remote=(learned?n[11]:n[4])+offset+(write && !normalizedWrite?98304:0);
  unsigned route=learned?n[12]:n[8];
  const uint32_t expect[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expect[i]);
  if(write)assert(linkNormStores==5120);
  for(unsigned i=0;i<bytes/4;i++){
   if(write)Put(route,remote+i*4,Mem(local+i*4));else Mem(local+i*4)=Gddr(route,remote+i*4);
  }
  Retire(bytes,write);
 }
 void EnterLink(){
  assert(!entered && pc==linkEntry && linkCaptures==128 && !Mem(0x17ffc8));
  // Inspect the REAL completed layer before any link read. No fake
  // terminal is written: this is an actual subroutine return, not a new
  // native launch or claim that the top-level ROOT has parked already.
  VerifyLayer(false);
  for(unsigned i=0;i<5120;i++)finalHidden[i]=Gddr(manifest[9],manifest[8]+i*4);
  for(unsigned i=0;i<8;i++)linkBefore[i]=counts[i];
  for(unsigned w=0;w<119;w++)peerSequence[w]=Peer(w).memory.at(0x74144);
  entered=true;puts("ACTUAL_JOINED_LAYER_RETURNED_TO_LINK all5120hidden observed/ALL119peer sequences retained;NO host arithmetic feedback/readback/relaunch");
 }
 bool RunLinked(){while(pc!=parkPc && !(unknown && l1[Index(pc)]==0x0000006f)){if(pc==linkEntry)EnterLink();StepRoot();}return !unknown;}
 void VerifyLink(bool terminal=true){
  assert(entered && linkPackets==8 && linkNormStores==5120 && !pending && !unknown && minStack>=0x6e000);
  if(terminal)assert(Mem(0x17ffc8)==TT_Q38_LAYER_LINK_MARKER && Mem(0x17ffc0)==donePc && Mem(0x17ffc4)==5120);
  double square=0;for(unsigned i=0;i<5120;i++){float x=Float(finalHidden[i]);square+=double(x*x);
   assert(Gddr(manifest[9],manifest[8]+98304+i*4)==finalHidden[i]);}
  float scale=1.f/sqrtf(float(square/5120)+1e-6f);const auto* n=link.Descriptor()+64;
  for(unsigned i=0;i<5120;i++)assert(Gddr(n[8],n[4]+i*4)==Bits((Float(finalHidden[i])*scale)*Float(Gddr(n[12],n[11]+i*4))));
  std::array<uint32_t,96> receipt;for(unsigned i=0;i<96;i++)receipt[i]=Mem(TT_Q38_LAYER_LINK_RECEIPT+i*4);
  assert(PioneerQ38ResidentLayerLinkReceipt(receipt.data(),link.Descriptor(),plan->LayerManifest(),linkBefore.data(),counts,command));
  for(unsigned w=0;w<119;w++)assert(!Peer(w).active && Peer(w).pc==TT_Q38_PEER_WAIT_LOAD && Peer(w).memory.at(0x74144)==peerSequence[w]);
 }
};
#ifndef PIONEER_Q38_LAYER_LINK_JOINED_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 puts("BEGIN ACTUAL complete recurrent layer->next learned RMS one uninterrupted ROOT/ALL119retained peers;SYNTHETIC raw/learned inputs only");
 OutputParts geometry(0,0);LinkedLayerFabric f(argv[1],argv[2],geometry);
 assert(f.RunLinked());f.VerifyLink();f.Inert();
 printf("PASS ACTUAL joined layer->next-input device handoff ALL5120 normalized+raw BITEXACT independent CPU observer/8NoC packets/ALL119peers unchanged/no CPU product injection/no host roundtrip/no relaunch/maxStack%u;NOT native/original-model/silicon/full64layer/token-rate qualification\n",0x70000-f.minStack);
}
#endif
