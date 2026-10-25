/* SPDX-License-Identifier: MIT */
// Actual complete NEW linked ROOT and ALL119 retained peer interpreters.
// Only original synthetic INPUT/learned bytes and NoC endpoints are modeled.
// No completed producer,CPU intermediate product,ready flag or sequence reset.
#define PIONEER_Q38_REF_DIAGNOSTIC
#define PIONEER_Q38_REFERENCE_ROUNDING_BOUND
#define PIONEER_Q38_RESIDENT_OUTPUT_COHORT_MACHINE_ONLY
#include "pioneer_q38_resident_output_cohort_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerPlan.h"
struct LayerFabric;
struct LayerPeer:CohortPeer {
 LayerFabric* layerFabric=nullptr;
 bool ffn=false,inputFfn=false;
 unsigned ffnRole=0,ffnPrepared=0,ffnVerified=0,matrixJobs=0,producerJobs=0;
 uint64_t ffnArithmetic=0;
 // Move the COMPLETE interpreter,including physical tensors/registers/PC/
 // private sequence. No reboot or fresh Machine is constructed here.
 explicit LayerPeer(CohortPeer&& p):CohortPeer(std::move(p)){}
 void PrepareFfn(unsigned role);
 void SubmitActivation()override;
 void StepBlock()override;
 void VerifyInputFfn();
};
struct LayerFabric:OutputCohortFabric {
 std::array<std::vector<uint8_t>,2> extra;
 std::unique_ptr<PioneerQ38ResidentLayerPlan> plan=std::make_unique<PioneerQ38ResidentLayerPlan>();
 std::array<uint32_t,5120> rawBefore={},projected={},normalized={},residual={},down={};
 std::array<std::vector<uint32_t>,3> expected;
 unsigned layerCaptures=0,normPackets=0,ffnCaptures=0,ffnMatrices=0,ffnProducers=0;
 unsigned assembled[3]={},normSumStores=0,normStores=0,residualPackets=0,finalSumStores=0;
 unsigned runtimeFault=0,prefixPackets=0,injectedRootPacket=0,pendingStackStores=0;
 bool prefixDone=false,normDone=false,ffnStarted=false;
 unsigned ffnEntry=0;
 LayerFabric(const char* image,const char* worker,const OutputParts& geometry,unsigned failure=0)
  :OutputCohortFabric(image,worker,geometry),runtimeFault(failure){
  assert(runtimeFault<=3);
  extra[0]=Read((std::string(image)+"/norm.bin").c_str());extra[1]=Read((std::string(image)+"/ffn.bin").c_str());
  assert(extra[0].size()==5404 && extra[1].size()==30496);
  ffnEntry=Symbol(image,"q38_resident_layer_ffn_once");
  std::array<uint32_t,1920> src;std::array<uint32_t,119> seq={};
  for(unsigned i=0;i<src.size();i++)src[i]=manifest[TT_Q38_COHORT_SOURCE_WORD+i];
  assert(plan->Build(*canonical,geometry.layer,geometry.position,0,0,248319,17,{1,2,3,4},src.data(),seq.data()));
  // The true existing prefix geometry,not a replacement completed prefix.
  for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)assert(plan->Manifest()[i]==manifest[i]);
  for(unsigned i=0;i<1152;i++)assert(plan->PostManifest()[i]==post[i]);
  for(unsigned i=0;i<TT_Q38_RESIDENT_LAYER_WORDS;i++)Mem(TT_Q38_RESIDENT_LAYER_MANIFEST+i*4)=plan->LayerManifest()[i];
  for(unsigned i=0;i<5120;i++){
   rawBefore[i]=Bits(.5f+float((i*17)%127)/256.f);
   Put(manifest[9],manifest[8]+98304+i*4,rawBefore[i]);
   Put(plan->LayerManifest()[14],plan->LayerManifest()[13]+i*4,Bits(.75f+float((i*31)%127)/256.f));
  }
  for(unsigned w=0;w<119;w++){
   auto p=std::make_unique<LayerPeer>(std::move(*peers[w]));
   p->layerFabric=this;assert(p->fabric==this && !p->active && p->pc==TT_Q38_PEER_WAIT_LOAD
    && !p->memory.at(0x74144) && !p->matrixInstructions && !p->requests);peers[w]=std::move(p);
  }
  // NoC endpoint object links follow the moved interpreters;their actual
  // tensors/registers/source bytes/private sequences are unchanged.
  for(unsigned w=0;w<96;w++)peers[w]->peer=peers[w^1].get();
  for(unsigned role=0;role<3;role++)expected[role].assign(role==2?5120:17408,0x7fc00000);
 }
 LayerPeer& Peer(unsigned w){return static_cast<LayerPeer&>(*peers.at(w));}
 unsigned ExtraSection(unsigned at)const{return Range(at,0x158000,unsigned(extra[0].size()))?0:Range(at,0x15c000,unsigned(extra[1].size()))?1:2;}
 uint32_t ExtraWord(unsigned at)const{unsigned i=ExtraSection(at);assert(i<2);return Word(extra[i],at-(i?0x15c000:0x158000));}
 bool CodeAddress(uint32_t at)const override{return ExtraSection(at)<2 || OutputCohortFabric::CodeAddress(at);}
 unsigned Index(uint32_t at)override{if(ExtraSection(at)<2){l1[8178]=ExtraWord(at);return 8178;}return OutputCohortFabric::Index(at);}
 bool RangeData(unsigned at)const override{return OutputCohortFabric::RangeData(at)
  || Range(at,TT_Q38_RESIDENT_LAYER_MANIFEST,TT_Q38_RESIDENT_LAYER_WORDS*4)
  || Range(at,TT_Q38_RESIDENT_LAYER_SHADOW,TT_Q38_RESIDENT_LAYER_WORDS*4)
  || Range(at,0x74000,0x38000);}
 uint32_t Load(uint32_t at)override{return ExtraSection(at)<2?ExtraWord(at):OutputCohortFabric::Load(at);}
 void VerifyPrefix(){
  assert(!prefixDone && gdnNumericsVerified && preparedVerified && !pending && !unknown);
  assert(convLaunches==119 && gdnLaunches==96 && outputLaunches==24 && matrixLaunches==119
   && lineReads==24 && compactWrites==1 && nativeReads==1 && scaleReads==1 && resultWrites==119);
  for(unsigned w=0;w<119;w++){
   auto& p=Peer(w);unsigned seq=w<24?4:w<96?3:2;
   assert(p.GdnFinished(seq) && !p.active && p.CorrectRows() && !p.dotProbeReads
    && Mem(TT_Q38_COHORT_SEQUENCE+w*4)==seq);
   unsigned first=post[1016+w];
   for(unsigned i=0;i<p.RowsMachine::groups*16;i++){
    assert(coverage[first+i]==1);projected[first+i]=Gddr(manifest[9],manifest[8]+TT_Q38_PROJECTION_OUTPUT_OFFSET+(first+i)*4);
    assert(projected[first+i]==p.buffers[4096+i]);
   }
   p.ffnPrepared=p.ffnVerified=seq;
  }
  prefixDone=true;prefixPackets=packets;
  // Deliberately corrupt an ORIGINAL raw norm input AFTER the genuine
  // completed prefix. No CPU numeric product/receipt is supplied.
  if(runtimeFault==1)Put(manifest[9],manifest[8]+98304+5119*4,0x7fc00001);
  puts("ACTUAL_LAYER_PREFIX_DONE Conv640/GDN96/QOL24/matrix119 ALL5120rows observed;NO CPU products fed back");
 }
 void VerifyNorm(){
  assert(prefixDone && !normDone && normPackets==10 && normSumStores==5120 && normStores==5120);
  double square=0;
  for(unsigned i=0;i<5120;i++){residual[i]=Bits(Float(rawBefore[i])+Float(projected[i]));float x=Float(residual[i]);square+=double(x*x);
   assert(Gddr(manifest[9],manifest[8]+98304+i*4)==residual[i]);}
  float scale=1.f/sqrtf(float(square/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++){
   uint32_t weight=Gddr(plan->LayerManifest()[14],plan->LayerManifest()[13]+i*4);
   normalized[i]=Gddr(manifest[9],manifest[8]+i*4);
   assert(normalized[i]==Bits((Float(residual[i])*scale)*Float(weight)));
  }
  normDone=true;puts("ACTUAL_LAYER_NORM_DONE ALL5120 residual+normalized BITEXACT independent observer;only actual device output consumed next");
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!Range(at,TT_Q38_RESIDENT_LAYER_MANIFEST,TT_Q38_RESIDENT_LAYER_WORDS*4));
  if(Range(at,TT_Q38_RESIDENT_LAYER_SHADOW,TT_Q38_RESIDENT_LAYER_WORDS*4)){
   assert(!packets && at==TT_Q38_RESIDENT_LAYER_SHADOW+layerCaptures*4
    && value==Mem(TT_Q38_RESIDENT_LAYER_MANIFEST+layerCaptures*4));layerCaptures++;Mem(at)=value;return;
  }
  if(at==0x17ffc8 && value==TT_Q38_RESIDENT_LAYER_FAULT){unknown=true;Mem(at)=value;return;}
  if(at==TT_Q38_COHORT_STATS+9*4 && value==5120){OutputCohortFabric::Store(at,value);VerifyPrefix();return;}
  if(!prefixDone){OutputCohortFabric::Store(at,value);return;}
  if(at==TT_Q38_COHORT_STATS+10*4 && value==5120){VerifyNorm();}
  if(Range(at,TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST,TT_Q38_FFN_RESIDUAL_GRAPH_WORDS*4))assert(normDone && !ffnStarted);
  if(Range(at,TT_Q38_FFN_RESIDUAL_GRAPH_SHADOW,TT_Q38_FFN_RESIDUAL_GRAPH_WORDS*4)){
   assert(normDone && !ffnStarted && at==TT_Q38_FFN_RESIDUAL_GRAPH_SHADOW+ffnCaptures*4
    && value==Mem(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+ffnCaptures*4));ffnCaptures++;
  }
  if(Range(at,0xffb20000,40)){assert(!pending && !unknown);command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending && !unknown);Submit();return;}
  // Actual function frames may unwind/call the terminal while a submitted
  // NoC transfer is UNKNOWN. Permit ONLY that private local stack,never
  // commands,receipts,GDDR,code or a manufactured transfer completion.
  if(pending && Range(at,0x6e000,8192)){
   assert(!unknown && !(at&3));pendingStackStores++;minStack=std::min(minStack,at);Mem(at)=value;return;
  }
  assert(!CodeAddress(at) && !(at&3) && RangeData(at) && !pending && !unknown);
  assert(!Range(at,TT_Q38_COHORT_MANIFEST,TT_Q38_COHORT_WORDS*4)
   && !Range(at,TT_Q38_OUTPUT_COHORT_MANIFEST,1152*4));
  if(!normDone){normSumStores+=Range(at,0x98000,20480);normStores+=Range(at,0xa0000,20480);}
  if(ffnStarted && Range(at,0x99000,20480))finalSumStores++;
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,at);
  Mem(at)=value;
 }
 void NormPacket(){
  unsigned p=normPackets++;assert(p<10);bool write=p>=6,learned=p>=4 && p<6,rawRead=p>=2 && p<4,normalizedWrite=p>=8;
  unsigned offset=(p&1)*16384,bytes=p&1?4096:16384;
  const auto* l=plan->LayerManifest();
  unsigned local=(write?(normalizedWrite?0xa0000:0x98000):learned?0x90000:rawRead?0x88000:0x80000)+offset;
  unsigned remote=(learned?l[13]:manifest[8])+(learned?0U:write?(normalizedWrite?0U:98304U):rawRead?98304U:1048576U)+offset;
  unsigned route=learned?l[14]:manifest[9];
  const uint32_t expect[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expect[i]);
  if(write)assert(normSumStores==5120 && normStores==5120);
  // Model UNKNOWN,not a successful zero-length transfer. No payload,
  // counter or completion is manufactured after the submitted request.
  if(runtimeFault==2 && p==6){pending=true;injectedRootPacket=packets;return;}
  for(unsigned i=0;i<bytes/4;i++){if(write)Put(route,remote+i*4,Mem(local+i*4));else Mem(local+i*4)=Gddr(route,remote+i*4);}
  Retire(bytes,write);
 }
 void Submit()override{
  if(!prefixDone){OutputCohortFabric::Submit();return;}
  packets++;if(packets==failPacket){pending=true;return;}
  if(!normDone){NormPacket();return;}
  assert(ffnCaptures==TT_Q38_FFN_RESIDUAL_GRAPH_WORDS);ffnStarted=true;
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(!command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
  if(route==manifest[9]){
   assert(remote>=manifest[8] && bytes && bytes<=303104 && remote-manifest[8]<=303104-bytes && !(bytes&63));
   if(Range(local,0x94000,40960))residualPackets++;
   for(unsigned i=0;i<bytes/4;i++){if(write)Put(route,remote+i*4,Mem(local+i*4));else Mem(local+i*4)=Gddr(route,remote+i*4);}
  }else{
   unsigned w=1;while(w<120 && PioneerQ38ConvPeer(w)!=route)w++;assert(w<120);
   auto& p=Peer(w-1);assert(local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
   if(remote>=0xffb00000)assert(!write && bytes==4);
   if(write){
    assert(!p.active && p.pc==TT_Q38_PEER_WAIT_LOAD && p.ffnPrepared==p.ffnVerified);
    assert((remote==0x74000 && bytes==64) || (remote==0x74200 && bytes==256) || (remote==0x74100 && bytes==4));
    for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=Mem(local+i*4);
    if(remote==0x74100)p.PrepareFfn(Mem(0x7f914));
   }else if(remote==0x74140){assert(bytes==64);for(unsigned i=0;i<16;i++)Mem(local+i*4)=p.memory.at(remote+i*4);}
   else if(remote==0xffb13140)Mem(local)=p.pc;
   else if(remote==0xffb121b0)Mem(local)=0x46800;
   else if(remote==0x83f00){
    assert(!p.active && p.ffnPrepared==p.ffnVerified && bytes==(p.memory.at(0x74034)==1?256+p.RowsMachine::groups*64:256));
    for(unsigned i=0;i<bytes/4;i++)Mem(local+i*4)=i<64?p.memory.at(remote+i*4):p.buffers[4096+i-64];
   }else{
    for(unsigned i=0;i<bytes/4;i++){
     unsigned at=remote+i*4;bool found=false;
     for(unsigned k=0;k<16;k++)if(at==0xffb20000+SrcOffset(k)){Mem(local+i*4)=p.source[k];found=true;}
     for(unsigned k=0;k<8;k++)if(at==0xffb20000+Offset(k)){Mem(local+i*4)=p.Count(k);found=true;}
     assert(found);
    }
   }
  }
  Retire(bytes,write);
 }
 void RecordFfn(LayerPeer& p){
  if(p.memory.at(0x74034)==1){
   const auto* f=plan->LayerManifest()+64;const auto* r=f+PioneerQ38FfnResidualGraphRoleOffset(p.ffnRole);
   unsigned first=r[16+119*8+p.logicalWorker-1];
   for(unsigned i=0;i<p.RowsMachine::groups*16;i++)expected[p.ffnRole].at(first+i)=p.buffers[4096+i];
   assembled[p.ffnRole]+=p.RowsMachine::groups*16;ffnMatrices++;
   if(p.logicalWorker==1 || p.logicalWorker==119)printf("ACTUAL_LAYER_FFN_MATRIX role%u worker%u seq%u;NOT_NATIVE_TIME\n",p.ffnRole,p.logicalWorker,p.ffnVerified);
  }else ffnProducers++;
 }
 bool RunLayer(){while(pc!=parkPc && !(unknown && l1[Index(pc)]==0x0000006f))StepRoot();return !unknown;}
 void VerifyLayer(){
  assert(prefixDone && normDone && ffnStarted && !unknown && !pending && layerCaptures==3520 && ffnCaptures==3456);
  assert(Mem(0x17ffc8)==TT_Q38_RESIDENT_LAYER_MARKER && Mem(0x17ffc4)==5120 && Mem(0x17ffc0)==donePc);
  assert(ffnMatrices==357 && ffnProducers==88 && residualPackets==6 && finalSumStores==5120);
  for(unsigned role=0;role<3;role++){
   assert(assembled[role]==expected[role].size());unsigned offset=role==0?30848:role==1?100480:274880;
   for(unsigned i=0;i<expected[role].size();i++)assert(isfinite(Float(expected[role][i])) && Gddr(manifest[9],manifest[8]+offset+i*4)==expected[role][i]);
  }
  for(unsigned i=0;i<5120;i++)assert(Gddr(manifest[9],manifest[8]+i*4)==Bits(Float(residual[i])+Float(expected[2][i]))
   && Mem(0x94000+i*4)==residual[i]);
  std::array<uint32_t,3456> active;
  for(unsigned i=0;i<3456;i++)active[i]=Mem(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+i*4);
  for(unsigned w=0;w<119;w++){
   auto& p=Peer(w);unsigned want=PioneerQ38FfnResidualGraphNextPeer(active.data(),w);
   assert(p.ffn && !p.active && !p.pending && p.pc==TT_Q38_PEER_WAIT_LOAD && p.ffnPrepared==want && p.ffnVerified==want
    && Mem(TT_Q38_COHORT_SEQUENCE+w*4)==want && p.matrixJobs==3 && !p.dotProbeReads);p.ClearTensorState();
  }
  assert(Mem(TT_Q38_COHORT_STATS+10*4)==5120 && Mem(TT_Q38_COHORT_STATS+12*4)==5120
   && Mem(TT_Q38_COHORT_STATS+17*4)==TT_Q38_RESIDENT_LAYER_MARKER);
 }
};
void LayerPeer::PrepareFfn(unsigned role){
 assert(layerFabric && !active && pc==TT_Q38_PEER_WAIT_LOAD && ffnPrepared==ffnVerified);ClearTensorState();
 assert(memory.at(0x74030)==launched+1 && memory.at(0x74100)==launched+1);
 uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=Count(i);
 NextPackets();for(unsigned i=0;i<8;i++)countBase[i]=independentBefore[i]=before[i];
 launched=ffnPrepared=memory.at(0x74100);ffn=true;gdnActive=convActive=outputActive=false;
 unsigned op=memory.at(0x74034);assert(op==1 || op==3 || op==4);ffnRole=role;
 activation=op!=1;inputFfn=op==4;activationPackets=commandReads=activatedWrites=nativeWrites=scaleWrites=0;
 native.fill(0xa5a5a5a5);scale.fill(0xa5a5a5a5);
 if(activation){for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(0x74200+i*4);
  if(inputFfn && logicalWorker==1 && layerFabric->runtimeFault==3)failPacket=3;
  active=true;return;}
 assert(role<3);const auto* r=layerFabric->plan->LayerManifest()+64+PioneerQ38FfnResidualGraphRoleOffset(role);
 const auto* part=r+16+(logicalWorker-1)*8;
 const uint32_t job[]={0x31483851,12,2304,0,0,r[3],part[0],part[2],part[4],1,TT_Q38_FFN_PERSISTENT_MARKER,logicalWorker,ffnPrepared,1,1,0};
 for(unsigned i=0;i<16;i++)assert(memory.at(0x74000+i*4)==job[i]);
 type=12;inputs=r[3];RowsMachine::groups=part[2];base=part[0];count=256;
 vector.resize(inputs);scales.resize(inputs/256);tile.assign(2304,0);tiles.assign(RowsMachine::groups*(inputs/256),tile);
 // ORIGINAL SYNTHETIC INPUT weights only;never a matrix result or wire.
 for(unsigned t=0;t<tiles.size();t++)for(unsigned row=0;row<16;row++){
  unsigned q=1+(t+row+logicalWorker+role)%3;uint16_t d=uint16_t(role==2?0x0100:role==0?0x0800:0x0c00);
  uint8_t raw[144]={};raw[0]=uint8_t(d);raw[1]=uint8_t(d>>8);
  for(unsigned i=0;i<4;i++)raw[4+i]=raw[12+i]=1;
  for(unsigned i=16;i<144;i++)raw[i]=uint8_t(q|(q<<4));
  for(unsigned i=0;i<144;i++)tiles[t][i*16+row]=raw[i];
 }
 root.clear();packets.clear();
 for(unsigned at=0;at<inputs*2;){unsigned bytes=std::min(16384U,inputs*2-at);packets.push_back({0xa0000+at,129,0xa0000+at,bytes,-1});
  for(unsigned i=0;i<bytes;i+=4)root[0xa0000+at+i]=layerFabric->Mem(0xa0000+at+i);at+=bytes;}
 unsigned padded=(unsigned(scales.size()*4)+63)&~63U;packets.push_back({0xab000,129,0x94000,padded,-1});
 for(unsigned at=0;at<padded;at+=4)root[0xab000+at]=layerFabric->Mem(0xab000+at);
 inputReads=unsigned(packets.size());
 for(unsigned t=0;t<tiles.size();t++)packets.push_back({base+t*2304,part[4],0x90000,2304,int(t)});
 for(unsigned i=0;i<inputs;i++)vector[i]=uint16_t(root.at(0xa0000+i/2*4)>>((i%2)*16));
 for(unsigned i=0;i<scales.size();i++)scales[i]=root.at(0xab000+i*4);
 Poison();PoisonTail();for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;active=true;
}
void LayerPeer::SubmitActivation(){
 if(!ffn){CohortPeer::SubmitActivation();return;}
 ClearTensorState();unsigned p=activationPackets;assert(p<(inputFfn?3U:4U));
 bool write=inputFfn?p!=0:p>=2;unsigned index=inputFfn?(p==0?10:p==1?12:13):10+p;
 unsigned local=inputFfn?(p==0?0x90000:p==1?0x90c00:0x90e00):p==0?0x90000:p==1?0x90400:p==2?0x90c00:0x90e00;
 unsigned bytes=inputFfn?(p==0?1024:p==1?512:64):p<2?1024:p==2?512:64;
 unsigned remote=descriptor[4]+descriptor[index];
 const uint32_t expect[]={write?local:remote,0,write?selfRoute:descriptor[8],write?remote:local,0,write?descriptor[8]:selfRoute,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)assert(command[i]==expect[i]);
 if(write)assert(nativeWrites==128 && scaleWrites==31 && activatedWrites==(inputFfn?0U:256U));
 if(p+1==failPacket){activationPackets++;pending=true;return;}
 for(unsigned i=0;i<bytes/4;i++){
  if(!write){uint32_t v=layerFabric->Gddr(descriptor[8],remote+i*4);memory[local+i*4]=v;(p?up:gate)[i]=v;}
  else{uint32_t v=memory.at(local+i*4);layerFabric->Put(descriptor[8],remote+i*4,v);(local==0x90c00?native.data():scale.data())[i]=v;}
 }
 activationPackets++;unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
}
void LayerPeer::VerifyInputFfn(){
 assert(activationPackets==3 && !activatedWrites && nativeWrites==128 && scaleWrites==31);
 float values[256];uint32_t q[73];for(unsigned i=0;i<256;i++)values[i]=Float(gate[i]);Q38ActivationReference(values,q);
 for(unsigned i=0;i<128;i++){uint32_t word=0;for(unsigned lane=0;lane<2;lane++){
  unsigned at=i*2+lane,byte=(q[1+at/4]>>((at%4)*8))&255;int v=int(byte)-int((byte&128)*2);
  word|=uint32_t(PioneerInt8Operand(v))<<(lane*16);}assert(native[i]==word);}
 assert(scale[0]==q[0] && PioneerQ38FfnInputScale(scale.data(),descriptor.data(),logicalWorker-1));
 uint32_t proof[64],job[16],after[8];for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
 for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);for(unsigned i=0;i<8;i++)after[i]=Count(i);
 assert(PioneerQ38FfnPersistentInputProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after));
}
void LayerPeer::StepBlock(){
 CohortPeer::StepBlock();if(!ffn || !active || pc!=TT_Q38_PEER_WAIT_LOAD || memory.at(0x74144)!=ffnPrepared)return;
 assert(!pending && memory.at(0x74140)==ffnPrepared && !memory.at(0x74148));ClearTensorState();
 if(!activation){assert(requests==packets.size() && CorrectRows() && !dotProbeReads);matrixJobs++;ffnArithmetic+=matrixInstructions;}
 else {if(inputFfn)VerifyInputFfn();else VerifyActivation();producerJobs++;}
 ffnVerified=ffnPrepared;active=false;layerFabric->RecordFfn(*this);
}
int main(int argc,char** argv){
 assert(argc==3 || argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 if(argc==4 && !strcmp(argv[3],"--preflight-faults")){
  unsigned faults=0;
  for(unsigned word:{0U,3U,13U,16U,40U,64U,70U,128U,272U,3519U,~0U}){
   OutputParts geometry(0,0);LayerFabric f(argv[1],argv[2],geometry);
   if(word==~0U)f.source[11]^=1;else f.Mem(TT_Q38_RESIDENT_LAYER_MANIFEST+word*4)^=1;
   assert(!f.RunLayer() && f.unknown && !f.packets && !f.captures && !f.postCaptures
    && f.layerCaptures==TT_Q38_RESIDENT_LAYER_WORDS && !f.convLaunches && !f.gdnLaunches
    && !f.normPackets && !f.ffnCaptures && !f.ffnMatrices && !f.ffnProducers);
   f.Inert();for(const auto& p:f.peers)assert(!p->active && p->pc==TT_Q38_PEER_WAIT_LOAD && !p->memory.at(0x74144));
   faults++;
  }
  printf("PASS ACTUAL NEW complete ROOT pre-I/O faults%u:bad layer/norm/source/output/FFN geometry/private-placeholder/late ROOTsource;complete immutable capture then fault park;ZERO ROOT packets/Conv/GDN/norm/FFN jobs,ALL119boot WAIT retained,inert/no reset/STOP/retry;NO native/transport/full-runtime-fault qualification\n",faults);return 0;
 }
 if(argc==4){
  unsigned kind=!strcmp(argv[3],"--runtime-fault1")?1:!strcmp(argv[3],"--runtime-fault2")?2:!strcmp(argv[3],"--runtime-fault3")?3:0;
  assert(kind);printf("BEGIN ACTUAL NEW joined ROOT runtime fault%u AFTER real prefix;SYNTHETIC endpoint fault,NOT hardware/native recovery evidence\n",kind);
  OutputParts geometry(0,0);LayerFabric f(argv[1],argv[2],geometry,kind);
  assert(!f.RunLayer() && f.unknown && f.prefixDone && !f.ffnMatrices && !f.finalSumStores);
  if(kind==1)assert(!f.normDone && f.normPackets==6 && f.packets==f.prefixPackets+6 && f.normSumStores==5119 && !f.normStores && !f.ffnCaptures && !f.pending);
  if(kind==2)assert(!f.normDone && f.normPackets==7 && f.packets==f.injectedRootPacket && f.pending && f.pendingStackStores && !f.ffnCaptures);
  if(kind==3){auto& p=f.Peer(0);assert(f.normDone && f.ffnStarted && f.normPackets==10 && f.residualPackets==2
   && p.pending && p.pc==TT_Q38_PEER_FAULT_PARK && p.activationPackets==3 && p.ffnPrepared==p.ffnVerified+1
   && p.memory.at(0x74144)==p.ffnVerified && f.ffnProducers<20);}
  unsigned before=f.packets,reads=f.nocReads;f.Inert();assert(f.packets==before && f.nocReads==reads);
  printf("PASS ACTUAL NEW joined ROOT runtime fault%u:real Conv/GDN/output prefix retained,ROOT fault park inert after detection/no FFN matrix/final residual/STOP/reset/retry;ROOTpackets%u normPackets%u actualFfnProducers%u;UNKNOWN owners still required,NO native failure-containment or silicon qualification\n",kind,f.packets,f.normPackets,f.ffnProducers);return 0;
 }
 puts("BEGIN ACTUAL NEW complete QLR1 ROOT+ALL119 SAME retained peers;SYNTHETIC original input/weights only;NO host intermediate products or fake producer");
 OutputParts geometry(0,0);LayerFabric f(argv[1],argv[2],geometry);
 if(!f.RunLayer()){fprintf(stderr,"LAYER_FAILURE pc%08x packets%u prefix%u norm%u matrices%u producers%u\n",f.pc,f.packets,f.prefixDone,f.normPackets,f.ffnMatrices,f.ffnProducers);return 1;}
 f.VerifyLayer();f.Inert();
 uint64_t math=0;for(unsigned w=0;w<119;w++)math+=f.Peer(w).ffnArithmetic;
 printf("PASS ACTUAL NEW complete recurrent-layer ROOT Conv640/GDN96/output24+119matrices/ALL5120 residual+RMS/FFN357matrixjobs+88producers/final5120residual BITEXACT/ALL119actual private WAIT sequences preserved/one ROOT launch/no peer reset/no host intermediate math/ROOTsteps%u packets%u FFNTensix%llu maxStack%u;SYNTHETIC_ENDPOINTS_NOT_NATIVE_OR_ORIGINAL_MODEL_OR_TOKEN_RATE\n",f.steps,f.packets,(unsigned long long)math,0x70000-f.minStack);
}
