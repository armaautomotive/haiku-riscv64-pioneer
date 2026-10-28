/* SPDX-License-Identifier: MIT */
// Whole persistent ROOT from reset, SAME119 virgin workers, ONE byte fabric.
// RAW learned/state/session inputs only. References OBSERVE and never feed
// emulated model products, completion flags or private processor history.
#define PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY
#include "pioneer_q38_global_chain_instruction_test.cpp"
#include "../PioneerQ38TokenStatePlan.h"
#include "../PioneerQ38TokenLayout.h"
namespace TL=PioneerQ38TokenLayout;namespace TS=PioneerQ38TokenStateLayout;
struct TokenFlowMachine:GlobalChainInstructionMachine {
 enum {Tokens=3,Prompts=2};
 const std::array<unsigned,Prompts> prompt={{248319,17}};
 std::array<uint32_t,1920> sources={};
 std::array<std::unique_ptr<PioneerQ38TokenStatePlan>,64> statePlan;
 std::vector<uint32_t> readState,priorKv;
 std::array<std::vector<uint32_t>,64> beforeConv;
 std::array<RetainedState,64> produced;
 std::array<std::array<uint32_t,64>,Tokens> records={};
 PioneerQ38InstructionWindowSet<Tokens*64> window;
 unsigned position=0,currentToken=248319,parent=0,allLayers=0,committed=0,flips=0,prepared=0;
 unsigned embedded=0,embeddingTarget=0,embeddingToken=0,embeddingStage=0,embeddingReads=0,embeddingWrites=0,embeddingBack=0;
 unsigned copyLayer=0,copySpan=0,copyAt=0,copyPhase=0,copyBytes=0,recordPhase=0;
 unsigned sourceCopies=0,stateWrites=0,stateReadbacks=0,tokenPublications=0;
 unsigned kvPhase=0,kvWrites=0,kvReadbacks=0;
 unsigned kvFaults=0,kvFault=0;bool kvInjected=false;
 bool expectCancelStop=false; // observer ONLY; never supplied to the device
 std::array<uint32_t,5120> expectedEmbedding={};
 static uint16_t ToHalf(float value){_Float16 h=_Float16(value);uint16_t bits;memcpy(&bits,&h,2);return bits;}
 static float FromHalf(uint16_t bits){_Float16 h;memcpy(&h,&bits,2);return float(h);}
 TokenFlowMachine(const char* image,const char* old,const char* worker):GlobalChainInstructionMachine(image,old,worker){
  assert(!steps && !packets && !peerWrites && feedback);
  for(unsigned i=0;i<16;i++)sources[i]=source[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->boot[16+i];
  unsigned workspace=outer[9];
  for(unsigned selected=0;selected<64;selected++)if(!CL::Dense(selected)){
   statePlan[selected]=std::make_unique<PioneerQ38TokenStatePlan>();assert(statePlan[selected]->Build(*canonical,selected,0));
   unsigned package=0;assert(CL::Address(workspace,selected,package));
   for(unsigned i=0;i<64;i++)Put(outer[8],package+TS::Offset+i*4,statePlan[selected]->Data()[i]);
  }
  uint32_t session[128]={};const unsigned h[]={TL::Magic,1,128,0,workspace,Tokens,Prompts,prompt[0]};
  for(unsigned i=0;i<8;i++){session[i]=h[i];session[24+i]=boot[8+i];}
  for(unsigned i=0;i<16;i++)session[8+i]=source[i];
  session[33]=248318;session[34]=248317;session[35]=8192; // EOS disabled: qualify all three positions
  assert(TL::Session(session,boot.data(),0,workspace));
  for(unsigned i=0;i<128;i++)Put(outer[8],workspace+TL::SessionOffset+i*4,session[i]);
  for(unsigned i=0;i<16;i++)Put(outer[8],workspace+TL::PromptOffset+i*4,i<Prompts?prompt[i]:0);
  // Immutable RAW positional constants, not normalized/rotated products.
  for(unsigned p=0;p<Tokens;p++)for(unsigned i=0;i<32;i++){
   float angle=float(p)*powf(10000000.f,-float(i)/32);
   Put(PioneerQ38ProjectionRoute(0),workspace+DL::RopeOffset+p*256+i*8,Bits(cosf(angle)));
   Put(PioneerQ38ProjectionRoute(0),workspace+DL::RopeOffset+p*256+i*8+4,Bits(sinf(angle)));
  }
  // No cache row is supplied as a CPU product or filled with convenient
  // zeros. Only the actual future dense-body append can create these bytes.
  unsigned rows=0;
  for(unsigned selected=3;selected<64;selected+=4)for(unsigned g=0;g<4;g++)for(unsigned which=0;which<2;which++){
   const auto* f=dense[selected]->Dense(g*24);bool canonicalRow=false;
   for(unsigned n=0;n<canonical->Count();n++){
    const auto* part=canonical->Part(n);unsigned row=g+which*4;
    if(part->kind==canonical->KvKind && part->layer==selected && row>=part->firstRow && row-part->firstRow<part->rows){
     assert(!canonicalRow && f[12+which]==PioneerQ38ProjectionRoute(part->bank)
      && f[8+which]==canonical->Base+part->offset+(row-part->firstRow)*4194304);canonicalRow=true;
    }
   }
   assert(canonicalRow);
   for(unsigned pos=0;pos<Tokens;pos++)for(unsigned i=0;i<128;i++)assert(!dram.count(Key(f[12+which],f[8+which]+pos*512+i*4)));
   rows++;
  }
  assert(rows==128 && !kvPhase && !kvWrites && !kvReadbacks);
 }
 unsigned Private(unsigned index)const{assert(parent>=0x6e000 && parent<=0x70000-1028);return parent+(192+index)*4;}
 unsigned ActualPrivate(unsigned index)const{return memory.at(Private(index));}
 void StartLayer(unsigned request){
  const unsigned nextPosition=releases/64,nextLayer=releases%64;
  assert(request==releases+1 && nextPosition<Tokens && committed==nextPosition);
  if(!nextLayer){
   assert(embedded==nextPosition+1 && embeddingStage==4 && !copyPhase && copyLayer==(nextPosition?64U:0U));
   if(nextPosition)for(unsigned w=0;w<119;w++){
    const auto& p=*child[w];unsigned expected=nonceBefore[w]+(w<21?9:8);
    assert(!p.active && p.GdnFinished(expected) && p.sequence==expected);nonceBefore[w]=expected;
   }
   position=nextPosition;currentToken=position<Prompts?prompt[position]:bestRow;
   assert(embeddingTarget==position && embeddingToken==currentToken);
   completed=globalJobs=globalPartials=globalRows=headQuant=0;bestRow=248320;bestBits=0;
   copyLayer=copySpan=copyAt=recordPhase=0;embeddingStage=0;
   for(unsigned i=0;i<5120;i++)input[i]=Get(outer[8],outer[9]+PioneerQ38GlobalFeedbackLayout::Offset+i*4);
   for(unsigned i=0;i<5120;i++)assert(input[i]==expectedEmbedding[i]);
  }
  layer=nextLayer;assert(completed==layer);releases++;
  const uint32_t* expected;
  if(!CL::Dense(layer)){
   auto p=std::make_unique<RPlan>();
   assert(p->Build(*canonical,layer,position,currentToken,0,position&1,1001+position*1024+layer*16,request,{1,2,3,4},sources.data(),0,
    outer[9]+(layer?unsigned(RB::FinalOffset):unsigned(PioneerQ38GlobalFeedbackLayout::Offset))));
   recurrent[layer]=std::move(p);expected=recurrent[layer]->Outer();
   readState.resize(96*8192);
   for(unsigned w=0;w<96;w++){
    const auto* st=recurrent[layer]->Gdn()+w*192+64;
    if(position)assert(st[12]==produced[layer].next[w][0] && st[5]==produced[layer].next[w][1]);
    for(unsigned i=0;i<8192;i++){
     readState[w*8192+i]=Get(st[12],st[5]+i*4);
     assert(readState[w*8192+i]==(position?produced[layer].state[w*8192+i]:0U));
    }
   }
   auto& history=beforeConv[layer];history.resize(10240*3);
   const auto* s=statePlan[layer]->Data();
   for(unsigned n=0;n<7;n++){
    const auto* d=s+8+n*8;for(unsigned i=0;i<d[5]/4;i++){
     history[d[2]*3+i]=Get(PioneerQ38ProjectionRoute(d[0]),d[1]+i*4);
     assert(history[d[2]*3+i]==(position?produced[layer].convolution[d[2]*3+i]:0U));
    }
   }
  }else{
   auto p=std::make_unique<PioneerQ38MixedLiveBlockPlan>();
   assert(p->Build(*canonical,layer,position,currentToken,0,1001+position*1024+layer*16,request,{1,2,3,4},sources.data(),0,outer[9]+RB::FinalOffset));
   dense[layer]=std::move(p);expected=dense[layer]->Outer();priorKv.clear();
   for(unsigned g=0;g<4;g++)for(unsigned which=0;which<2;which++){
    const auto* d=dense[layer]->Dense(g*24);
    for(unsigned t=0;t<position;t++)for(unsigned i=0;i<128;i++)priorKv.push_back(Get(d[12+which],d[8+which]+t*512+i*4));
   }
  }
  for(unsigned i=0;i<128;i++){outer[i]=memory.at(RB::Outer+i*4);assert(outer[i]==expected[i]);}
  if(!parent){parent=regs[27];assert(ActualPrivate(18)==0 && ActualPrivate(19)==0);}
  assert(regs[27]==parent && ActualPrivate(16)==position && ActualPrivate(17)==currentToken && ActualPrivate(18)==position && ActualPrivate(19)==(position&1));
  matrices=lastMatrix=finalWords=inputReads=kvPhase=0;
  printf("BEGIN actual persistent token%u/layer%u/inputToken%u/previous committed%u/SAME119 histories\n",position,layer,currentToken,committed);
 }
 std::array<float,256> Prepared(unsigned g,unsigned h,bool key){
  std::array<float,256> q;unsigned address=outer[9]+(key?TT_Q38_DENSE_K+g*1024:TT_Q38_DENSE_QG+g*12288+h*2048);
  unsigned bank=key?outer[14]:outer[12],weight=key?outer[15]:outer[13];double sum=0;
  for(unsigned i=0;i<256;i++){float x=Float(Get(outer[8],address+i*4));sum+=double(x*x);}
  float scale=1.f/sqrtf(float(sum/256)+1e-6f);
  for(unsigned i=0;i<256;i++)q[i]=(Float(Get(outer[8],address+i*4))*scale)*Float(Get(PioneerQ38ProjectionRoute(bank),weight+i*4));
  for(unsigned i=0;i<32;i++){
   float a=q[i],b=q[i+32],c=Float(Get(outer[41],outer[40]+position*256+i*8)),s=Float(Get(outer[41],outer[40]+position*256+i*8+4));
   q[i]=fmaf(a,c,-(b*s));q[i+32]=fmaf(a,s,b*c);
  }
  return q;
 }
 void ObservePrepared(){
  assert(CL::Dense(layer) && matrices==3);
  for(unsigned g=0;g<4;g++){
   for(unsigned h=0;h<6;h++){
    auto q=Prepared(g,h,false);
    for(unsigned i=0;i<256;i++){
     assert(memory.at(DL::Input+(g*1536+h*256+i)*4)==Bits(q[i]));
     assert(memory.at(DL::Gate+(g*1536+h*256+i)*4)==Get(outer[8],outer[9]+TT_Q38_DENSE_QG+g*12288+(h*512+256+i)*4));
    }
   }
   auto k=Prepared(g,0,true);
   for(unsigned i=0;i<128;i++){
    unsigned wantedK=ToHalf(k[i*2])|(uint32_t(ToHalf(k[i*2+1]))<<16);
    unsigned wantedV=ToHalf(Float(Get(outer[8],outer[9]+TT_Q38_DENSE_V+g*1024+i*8)))
     |(uint32_t(ToHalf(Float(Get(outer[8],outer[9]+TT_Q38_DENSE_V+g*1024+i*8+4))))<<16);
    assert(memory.at(DL::NewK+g*512+i*4)==wantedK && memory.at(DL::NewV+g*512+i*4)==wantedV);
   }
  }
  prepared++;
 }
 float Cached(unsigned g,unsigned which,unsigned t,unsigned i)const{
  const auto* d=dense[layer]->Dense(g*24);unsigned word=Get(d[12+which],d[8+which]+t*512+(i/2)*4);
  return FromHalf(uint16_t(word>>((i&1)*16)));
 }
 void ObserveAttention(){
  assert(CL::Dense(layer) && matrices==3 && kvPhase==16);double delta=0,norm=0;
  for(unsigned g=0;g<4;g++)for(unsigned which=0;which<2;which++)for(unsigned i=0;i<128;i++){
   const auto* d=dense[layer]->Dense(g*24);
   assert(Get(d[12+which],d[8+which]+position*512+i*4)==memory.at((which?DL::NewV:DL::NewK)+g*512+i*4));
  }
  for(unsigned g=0;g<4;g++)for(unsigned h=0;h<6;h++){
   std::vector<float> probability(position+1);float maximum=-INFINITY;double denominator=0;
   for(unsigned t=0;t<=position;t++){
    double sum=0;for(unsigned i=0;i<256;i++)sum+=double(float(_Float16(Float(memory.at(DL::Input+(g*1536+h*256+i)*4))))*Cached(g,0,t,i));
    probability[t]=float(sum)*.0625f;maximum=fmaxf(maximum,probability[t]);
   }
   for(auto& p:probability){p=expf(p-maximum);denominator+=double(p);}
   float inverse=float(1.0/denominator);for(auto& p:probability)p=float(_Float16(p*inverse));
   assert(memory.at(DL::Parameters+(g*16+h)*4)==Bits(maximum) && memory.at(DL::Parameters+(g*16+8+h)*4)==Bits(inverse));
   for(unsigned w=0;w<24;w++){
    const auto& p=*child[g*24+w];const auto& f=p.denseHeader;
    for(unsigned t=0;t<f[49];t++)assert(p.memory.at(MWD::Scores+(h*384+t)*4)==Bits(probability[f[48]+t]));
   }
   for(unsigned i=0;i<256;i++){
    double sum=0;for(unsigned t=0;t<=position;t++)sum+=double(probability[t]*Cached(g,1,t,i));
    float gate=Float(memory.at(DL::Gate+(g*1536+h*256+i)*4));double e=exp(-fabs(double(gate))),sigmoid=gate>=0?1/(1+e):e/(1+e);
    float wanted=float(sum)*float(sigmoid),actual=Float(memory.at(DL::Output+(g*1536+h*256+i)*4));assert(isfinite(actual));
    delta+=(double(actual)-wanted)*(double(actual)-wanted);norm+=double(wanted)*wanted;
   }
  }
  assert(sqrt(delta/(norm?norm:1))<4e-6);attentionChecks++;
 }
 void VerifyTokenLayer(){
  assert(completed==layer && matrices==(!CL::Dense(layer)?8U:7U) && finalWords==5120 && inputReads==5120);
  if(!CL::Dense(layer)){
   auto& saved=produced[layer];saved.route=outer[8];assert(PioneerQ38RecurrentHistoryLayout::Address(outer[9],layer,saved.history));
   saved.convolution.resize(10240*3);saved.state.resize(96*8192);
   for(unsigned i=0;i<saved.convolution.size();i++){
    saved.convolution[i]=Get(saved.route,saved.history+i*4);assert(saved.convolution[i]==Get(saved.route,outer[9]+TT_Q38_CONV_NEXT+i*4));
   }
   for(unsigned w=0;w<96;w++){
    const auto* st=recurrent[layer]->Gdn()+w*192+64;saved.next[w]={st[13],st[7]};
    for(unsigned i=0;i<8192;i++){
     assert(Get(st[12],st[5]+i*4)==readState[w*8192+i]);
     unsigned value=Get(st[13],st[7]+i*4);assert(isfinite(Float(value)));saved.state[w*8192+i]=value;
    }
   }
  }else{
   assert(kvPhase==16);
   unsigned at=0;
   for(unsigned g=0;g<4;g++)for(unsigned which=0;which<2;which++)for(unsigned t=0;t<position;t++)for(unsigned i=0;i<128;i++){
    const auto* d=dense[layer]->Dense(g*24);assert(Get(d[12+which],d[8+which]+t*512+i*4)==priorKv.at(at++));
   }
   assert(at==priorKv.size());
  }
  for(unsigned i=0;i<5120;i++)input[i]=Get(outer[8],outer[9]+RB::FinalOffset+i*4);
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];Advance(w);unsigned change=!CL::Dense(layer)?PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w):DL::RunDelta(w);
   assert(!p.active && p.GdnFinished(p.sequence) && p.sequence==nonceBefore[w]+change && memory.at(RB::Nonces+w*4)==p.sequence);nonceBefore[w]=p.sequence;
  }
  completed++;allLayers++;assert(window.RebaseAfterVerifiedLayer(steps,allLayers));
  printf("PASS actual persistent token%u/layer%u/all119 closed/final5120/priorGDN+KV unchanged/nextstate retained/ROOTinstructions%llu/reservedStack%u/writtenStack%u\n",position,layer,(unsigned long long)window.Total(steps),0x70000-minSp,0x70000-minStack);
 }
 void BeginEmbedding(){
  assert(embeddingStage==0 && embedded<Tokens && (embedded?completed==64 && globalJobs==973 && globalPartials==973:!releases));
  embeddingTarget=embedded;embeddingToken=embedded<Prompts?prompt[embedded]:bestRow;
  PioneerQ38EmbeddingPlan plan;assert(plan.Build(*canonical,embeddingToken,embeddingTarget,0,{1,2,3,4},source.data(),17));
  const auto* d=plan.Descriptor();
  for(unsigned block=0;block<20;block++){
   uint8_t packed[144];float values[256];
   for(unsigned b=0;b<144;b++){
    unsigned a=d[11]+block*2304+b*16+d[13];packed[b]=uint8_t(Get(d[12],a&~3U)>>((a&3)*8));
   }
   assert(PioneerDecodeQ4K(packed,values));for(unsigned i=0;i<256;i++)expectedEmbedding[block*256+i]=Bits(values[i]);
  }
  embeddingReads=embeddingWrites=embeddingBack=0;embeddingStage=1;
 }
 bool EmbeddingPacket(unsigned route,unsigned remote,unsigned bytes,unsigned local,bool write){
  namespace F=PioneerQ38GlobalFeedbackLayout;
  if(!write && bytes==2304){
   if(!embeddingStage)BeginEmbedding();
   assert(embeddingStage==1 && embeddingReads<20 && !embeddingWrites);
   uint32_t geometry[8];assert(F::Describe(embeddingToken,embeddingTarget,0,outer[9],geometry));
   assert(route==PioneerQ38ProjectionRoute(geometry[4]) && remote==geometry[5]+embeddingReads*2304);embeddingReads++;return true;
  }
  if(route!=outer[8] || remote<outer[9]+F::Offset || remote>=outer[9]+F::Offset+20480)return false;
  // Layer0's later model input reads are not new embedding publications.
  if(embeddingStage!=1 && embeddingStage!=2 && embeddingStage!=3)return false;
  assert(embeddingReads==20 && local==RB::Scratch);unsigned at=remote-outer[9]-F::Offset;
  unsigned ordinal=write?embeddingWrites:embeddingBack;
  assert(at==ordinal*16384 && ordinal<2 && bytes==std::min(16384U,20480-at));
  for(unsigned i=0;i<5120;i++)assert(memory.at(F::Local+i*4)==expectedEmbedding[i] && memory.at(F::Shadow+i*4)==expectedEmbedding[i]);
  for(unsigned i=0;i<bytes/4;i++)assert((write?memory.at(local+i*4):Get(route,remote+i*4))==expectedEmbedding[at/4+i]);
  if(write){assert(!embeddingBack);embeddingWrites++;embeddingStage=2;}
  else{
   assert(embeddingWrites==2);embeddingBack++;embeddingStage=3;
   if(embeddingBack==2){embedded++;embeddingStage=4;printf("PASS actual next embedding position%u/token%u/5120 bit-exact/20raw+2writes+2readbacks/no CPU product upload\n",embeddingTarget,embeddingToken);}
  }
  return true;
 }
 bool StatePacket(unsigned route,unsigned remote,unsigned bytes,unsigned local,bool write){
  if(completed!=64 || globalJobs!=973 || globalPartials!=973 || copyLayer>=64)return false;
  // Skip dense layers only in observer bookkeeping, never device control.
  while(copyLayer<64 && CL::Dense(copyLayer))copyLayer++;
  if(copyLayer==64)return false;
  const auto* s=statePlan[copyLayer]->Data();const auto* d=s+8+copySpan*8;
  unsigned sourceAddress=s[5]+d[2]*12+copyAt,destination=d[1]+copyAt,destinationRoute=PioneerQ38ProjectionRoute(d[0]);
  const bool from=route==outer[8] && remote==sourceAddress;
  const bool to=route==destinationRoute && remote==destination;
  if(!from && !to)return false;
  assert(local==RB::Scratch && !stopJobs && (embedded==position+2 || position+1==Tokens));
  assert(bytes==std::min(16384U,d[5]-copyAt));
  if(!copyPhase){assert(from && !write);copyPhase=1;copyBytes=bytes;sourceCopies++;}
  else if(copyPhase==1){assert(to && write && bytes==copyBytes);copyPhase=2;stateWrites++;}
  else{assert(copyPhase==2 && to && !write && bytes==copyBytes);copyPhase=0;stateReadbacks++;}
  const auto& expected=produced[copyLayer].convolution;
  for(unsigned i=0;i<bytes/4;i++)assert((write?memory.at(local+i*4):Get(route,remote+i*4))==expected[d[2]*3+copyAt/4+i]);
  if(!copyPhase){
   copyAt+=bytes;
   if(copyAt==d[5]){copyAt=0;if(++copySpan==7){copySpan=0;copyLayer++;while(copyLayer<64 && CL::Dense(copyLayer))copyLayer++;}}
  }
  return true;
 }
 void TokenPacket(unsigned route,unsigned remote,unsigned bytes,unsigned local,bool write){
  unsigned first=outer[9]+TL::OutputOffset;if(route!=outer[8] || remote<first || remote>=first+Tokens*256)return;
  assert(remote==first+position*256 && local==RB::Scratch && completed==64 && globalJobs==973 && globalPartials==973
   && copyLayer==64 && !copyPhase && flips==position+1 && ActualPrivate(18)==position+1 && ActualPrivate(19)==((position+1)&1) && !stopJobs);
  auto& expected=records[position];
  if(!recordPhase){
   assert(write && bytes==256);expected.fill(0);
   const unsigned h[]={0,1,position,currentToken,bestRow,bestBits,position+1,(position+1)&1,unsigned(position+1<Prompts),unsigned(position+1<Tokens),64,973};
   for(unsigned i=0;i<12;i++)expected[i]=h[i];
   for(unsigned i=0;i<8;i++){expected[12+i]=boot[8+i];expected[36+i]=counts[i];}
   for(unsigned i=0;i<16;i++)expected[20+i]=source[i];expected[44]=position+1;expected[45]=48;expected[46]=119;
  }else if(recordPhase==1)assert(!write && bytes==256);
  else if(recordPhase==2){assert(write && bytes==4);expected[0]=TL::TokenMagic;}
  else assert(recordPhase==3 && !write && bytes==256);
  for(unsigned i=0;i<bytes/4;i++)assert((write?memory.at(local+i*4):Get(route,remote+i*4))==expected[i]);
  if(++recordPhase==4){
   assert(kvWrites==(position+1)*128 && kvReadbacks==kvWrites);
   committed++;tokenPublications++;
   printf("PASS actual MODEL token%u committed/greedy%u/all48 Conv copies+readbacks/one GDN slot flip/KV frontier/token marker actual readback;cohort STILL OWNED,NOT native cleanup\n",position,bestRow);
  }
 }
 void KvPacket(unsigned route,unsigned remote,unsigned bytes,unsigned local,bool write){
  if(!releases || !CL::Dense(layer))return;
  bool row=false;
  for(unsigned g=0;g<4;g++)for(unsigned which=0;which<2;which++){
   const auto* f=dense[layer]->Dense(g*24);
   if(route==f[12+which] && remote>=f[8+which] && uint64_t(remote)<uint64_t(f[8+which])+4194304)row=true;
  }
  if(!row)return;
  assert(matrices==3 && kvPhase<16 && bytes==512 && local==RB::Scratch);
  unsigned group=kvPhase/4,which=(kvPhase/2)&1;
  const auto* f=dense[layer]->Dense(group*24);
  assert(route==f[12+which] && remote==f[8+which]+position*512 && write==!(kvPhase&1));
  unsigned from=(which?DL::NewV:DL::NewK)+group*512;
  // Deliberately corrupted child lifetimes must reach the ACTUAL device
  // guard, not be stopped by this host-only numeric observer first.
  if(!kvFault)for(unsigned i=0;i<128;i++)assert((write?memory.at(local+i*4):Get(route,remote+i*4))==memory.at(from+i*4));
  if(write)kvWrites++;else kvReadbacks++;kvPhase++;
 }
 void SubmitToken(){
  packets++;const bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0];
  unsigned route=write?command[5]:command[2],bytes=command[8];
  assert(bytes && bytes<=16384 && !(bytes&3) && !(remote&3) && !command[1] && !command[4] && !command[6] && !command[9]
   && (write?command[2]:command[5])==129 && (write || !command[7]));
  bool bank=false;for(unsigned b=0;b<7;b++)bank|=route==PioneerQ38ProjectionRoute(b);
  if(bank){
   assert(local==RB::Scratch+(remote&31) || (Range(pc,0x64000,unsigned(conv.size())) && Range(local,0x80000,0x1100)));
   const bool embedding=EmbeddingPacket(route,remote,bytes,local,write);
   const bool state=StatePacket(route,remote,bytes,local,write);assert(!embedding || !state);
   KvPacket(route,remote,bytes,local,write);
   TokenPacket(route,remote,bytes,local,write);
   if(releases && !write && remote==(!CL::Dense(layer)?outer[69]:outer[74])+inputReads*4 && (Phase()==1 || Phase()==2)){
    assert(inputReads+bytes/4<=5120);for(unsigned i=0;i<bytes/4;i++)assert(Get(route,remote+i*4)==input[inputReads+i]);inputReads+=bytes/4;
   }
   if(write && remote>=outer[9]+RB::FinalOffset && uint64_t(remote)+bytes<=uint64_t(outer[9])+RB::FinalOffset+20480){
    assert(remote==outer[9]+RB::FinalOffset+finalWords*4 && matrices==(!CL::Dense(layer)?8U:7U));
    unsigned down=!CL::Dense(layer)?PioneerQ38MixedRecurrentOffset(4):unsigned(DL::DownOffset);
    for(unsigned i=0;i<bytes/4;i++)assert(memory.at(local+i*4)==Bits(Float(Get(route,outer[9]+down+(finalWords+i)*4))+Float(memory.at(RB::PostResidual+(finalWords+i)*4))));
    finalWords+=bytes/4;
   }
   for(unsigned i=0;i<bytes/4;i++)if(write)Put(route,remote+i*4,memory.at(local+i*4));else memory[local+i*4]=Get(route,remote+i*4);
  }else{
   unsigned w=0;while(w<119 && M::Peer(w+1)!=route)w++;assert(w<119);auto& p=*child[w];Advance(w);
   assert(local==RB::Scratch+(remote&31) && bytes<=16384-(remote&31));
   if(write){
    assert(!p.active);if(remote==M::Job && memory.at(local+8)==1)MatrixObserver();
    for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
    if(remote==M::Doorbell){
     unsigned op=p.memory.at(M::Job+8);
     if(op==1)matrixJobs++;else if(op==7)convJobs++;else if(op==8)gdnJobs++;else if(op>=9 && op<=11)denseJobs++;
     else if(op==G::GlobalMatrix){assert(completed==64 && p.memory.at(M::Job+24)==position);if(!globalJobs)HeadInput();globalJobs++;}
     else{
      assert(op==2);
      if(expectCancelStop){
       unsigned stage=ActualPrivate(29);assert(cancelBoundary && stage>=1 && stage<=3 && completed==layer+1
        && committed==position+unsigned(stage==3) && p.memory.at(M::Job+5*4)==(stage==1?layer:64U)
        && p.memory.at(M::Job+6*4)==position);
      }else assert(committed==Tokens && globalJobs==973 && globalPartials==973);
      stopJobs++;
     }
     p.Receive();
    }
    peerWrites++;
   }else if(remote==0xffb121b0){assert(bytes==4);memory[local]=0x46800;}
   else if(remote==0xffb13140){assert(bytes==4);memory[local]=p.pc;}
   else if(Range(remote,0xffb20000,0x2000)){assert(bytes==4);memory[local]=p.Load(remote);}
   else if(remote==G::Partial){
    assert(bytes==512 && !p.active && p.memory.at(M::Job+8)==G::GlobalMatrix && p.CorrectRows() && !p.dotProbeReads && p.requests==p.packets.size());
    if(completed==64){
     assert(p.memory.at(M::Job+24)==position);unsigned first=p.memory.at(M::Job+28*4),rows=p.memory.at(M::Job+23*4)*16;
     unsigned best=0;float maximum=Float(p.buffers[4096]);assert(isfinite(maximum));
     for(unsigned i=1;i<rows;i++){float x=Float(p.buffers[4096+i]);assert(isfinite(x));if(x>maximum){best=i;maximum=x;}}
     assert(p.memory.at(G::Partial)==G::PartialMagic && p.memory.at(G::Partial+8*4)==first+best && p.memory.at(G::Partial+9*4)==p.buffers[4096+best]);
     if(bestRow==248320 || maximum>Float(bestBits) || (maximum==Float(bestBits) && first+best<bestRow)){bestRow=first+best;bestBits=p.buffers[4096+best];}
     globalPartials++;globalRows+=rows;
    }else{
     assert(position && !layer && committed==position && p.memory.at(M::Job+24)==position-1
      && p.memory.at(M::Job+27*4)==(w<21?8U:7U) && p.sequence==nonceBefore[w]);
    }
    for(unsigned i=0;i<128;i++)memory[local+i*4]=i<64?p.memory.at(G::Partial+i*4):p.memory.at(0x83f00+(i-64)*4);
   }else if(remote==0x83f00 && bytes>256){
    assert(!p.active && p.memory.at(M::Job+8)==1 && p.CorrectRows() && !p.dotProbeReads && p.requests==p.packets.size());
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=i<64?p.memory.at(remote+i*4):p.buffers[4096+i-64];
   }else for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)>>6;counts[write?7:3]++;counts[2]++;counts[6]++;
  if(kvFault==2 && !kvInjected && !write && kvPhase==2){memory[local]^=1;kvInjected=true;}
 }
 void Store(uint32_t a,uint32_t value)override{
  if(a==0xffb20040){assert(value==1 && !pending);SubmitToken();return;}
  if(parent && a==Private(19)){
   assert(value==((position+1)&1) && flips==position && completed==64 && globalJobs==973 && globalPartials==973
    && copyLayer==64 && !copyPhase && !stopJobs);flips++;
  }
  if(parent && a==Private(18))assert(value==position+1 && flips==position+1 && copyLayer==64 && !copyPhase);
  if(a==RB::Doorbell){StartLayer(value);memory[a]=value;return;}
  if(a==RB::Status+4){if(value==outer[48])VerifyTokenLayer();memory[a]=value;return;}
  if(a==RB::Status+20 && CL::Dense(layer) && value==5 && memory.at(a)!=5)ObservePrepared();
  if(a==RB::Status+20 && CL::Dense(layer) && value==6 && memory.at(a)!=6){ObserveAttention();memory[a]=value;return;}
  GlobalChainInstructionMachine::Store(a,value);
 }
 void StepToken(){
  uint64_t before=window.Total(steps);assert(before<PioneerQ38InstructionWindowSet<Tokens*64>::TotalLimit);profile.Record(pc);
  StepScalar();assert(window.Total(steps)==before+1 && profile.Total()==before+1);
  if(!kvFaults && !position && layer==3 && kvPhase==1){
   // Fork the genuine executed history AFTER its first real KV write.
   // No PC/register/private state is transplanted or replaced. The parent
   // waits unchanged while each child independently resumes the real ROM.
   assert(allLayers==3 && matrices==3 && kvWrites==1 && !kvReadbacks && !committed);
   kvFaults=1;fflush(nullptr);
   for(unsigned fault=1;fault<=9;fault++){
    pid_t pid=fork();assert(pid>=0);
    if(!pid){
     kvFault=fault;kvInjected=fault!=2;
     const auto* f=dense[layer]->Dense(0);
     if(fault==1)Put(f[12],f[8],Get(f[12],f[8])^1);
     if(fault==3)counts[7]++;
     if(fault==4)source[11]^=1;
     if(fault==5)command[5]^=1;
     if(fault==6)memory[RB::Outer+4*4]^=1;
     if(fault==7)memory[DL::NewK]^=1;
     if(fault==8)memory[DL::InputShadow+24576]^=1;
     if(fault==9)memory[DL::DenseCopies+8*4]^=64;
     return;
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
   }
   puts("PASS ACTUAL 9 KV append faults AFTER genuine first on-card row write/from SAME executed ROOT+119 histories/no forged processor state/parent unchanged;not whole-token or terminal proof");
  }
 }
 bool RunTokens(){
  unsigned previousPC=0;
  while(pc!=0x60000 && pc!=0x60004){previousPC=pc;cycles+=19;StepToken();if(!(steps&255))for(unsigned w=0;w<119;w++)Advance(w,64);}
  if(pc==0x60000)fprintf(stderr,"TOKEN_FAULT position%u layer%u completed%u phase%u matrices%u global%u/%u committed%u copy%u/%u/%u/%u packets%u SP%x\n",
   position,layer,completed,Phase(),matrices,globalJobs,globalPartials,committed,copyLayer,copySpan,copyAt,copyPhase,packets,regs[2]);
  if(pc==0x60000 && !kvFault)PioneerQ38InstructionFault::Dump(*this,previousPC,RB::Projected,RB::PostResidual,RB::Final,RB::Original);
  return pc==0x60004;
 }
 void VerifyKvFault(){
  assert(kvFault && kvInjected && kvFaults==1 && pc==0x60000 && !position && layer==3 && allLayers==3 && matrices==3
   && kvWrites==1 && kvReadbacks<=1 && kvPhase<=2 && !committed && !flips && !stopJobs);
  unsigned before=packets,writes=peerWrites;auto commands=command;auto physical=counts;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==0x60000 && packets==before && peerWrites==writes && command==commands && counts==physical);
  printf("PASS ACTUAL KV fault%u/scalar fault park/no second row or later projection/token commit/NoC+peer inert2048\n",kvFault);fflush(nullptr);
 }
 void VerifyTokens(){
  assert(pc==0x60004 && position==Tokens-1 && completed==64 && releases==Tokens*64 && allLayers==Tokens*64 && window.Closed()==Tokens*64
   && committed==Tokens && flips==Tokens && tokenPublications==Tokens && embedded==Tokens && copyLayer==64 && !copyPhase && recordPhase==4
   && matrixJobs==Tokens*46128 && convJobs==Tokens*5712 && gdnJobs==Tokens*4608 && denseJobs==Tokens*4608 && stopJobs==119
   && globalJobs==973 && globalPartials==973 && globalRows==248320 && headQuant==1 && rmsChecks==Tokens*128 && quantChecks==Tokens*496
   && activationChecks==Tokens*64 && gdnChecks==Tokens*48 && attentionChecks==Tokens*16 && prepared==Tokens*16
   && sourceCopies==Tokens*672 && stateWrites==sourceCopies && stateReadbacks==sourceCopies && kvWrites==Tokens*128 && kvReadbacks==kvWrites
   && kvFaults==1 && !kvFault && minSp>=0x6e000 && minStack>=0x6e000);
  const unsigned terminal[]={TL::TerminalMagic,1,1,Tokens,Tokens-1,119,64,973,Tokens&1,1};
  for(unsigned i=0;i<10;i++)assert(memory.at(TL::TerminalLocal+i*4)==terminal[i]);
  for(unsigned i=0;i<8;i++)assert(memory.at(TL::TerminalLocal+(12+i)*4)==boot[8+i] && memory.at(TL::TerminalLocal+(36+i)*4)==counts[i]);
  for(unsigned i=0;i<16;i++)assert(memory.at(TL::TerminalLocal+(20+i)*4)==source[i]);
  for(unsigned p=0;p<Tokens;p++)for(unsigned i=0;i<64;i++)assert(Get(outer[8],outer[9]+TL::OutputOffset+p*256+i*4)==records[p][i]);
  for(unsigned selected=0;selected<64;selected++)if(!CL::Dense(selected)){
   const auto& s=produced[selected];for(unsigned w=0;w<96;w++)for(unsigned i=0;i<8192;i++)assert(Get(s.next[w][0],s.next[w][1]+i*4)==s.state[w*8192+i]);
  }
  for(unsigned w=0;w<119;w++){
   const auto& p=*child[w];assert(p.pc==p.stopPC && p.sequence==nonceBefore[w]+(w<21?9:8)+1 && p.memory.at(M::Status+16)==2);
  }
  unsigned before=packets;for(unsigned i=0;i<2048;i++)StepToken();assert(packets==before && pc==0x60004);profile.Dump(window.Total(steps));
  printf("PASS ACTUAL persistent ROOT %u FULL tokens/two prompt inputs then genuine greedy feedback/SAME119 from virgin boot/ALL64+973 EACH/nonzero GDN+append-only KV consumed across positions/all48 Conv commits+actual readbacks/one slot flip and model record EACH/final119STOP/inert2048/stack reserved%u written%u;SYNTHETIC raw inputs,NOT original model/native/physical/Work/TPS\n",Tokens,0x70000-minSp,0x70000-minStack);
 }
};
#ifndef PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && !strcmp(argv[4],"--prepare-only")));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<TokenFlowMachine>(argv[1],argv[2],argv[3]);
 if(argc==5){
  assert(!m->steps && !m->packets && !m->peerWrites && !m->committed && !m->parent);
  for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));
  puts("PASS RAW persistent fixture/3 positions/2 prompt IDs/disjoint canonical state-copy DATA/raw RoPE constants/SAME119 virgin/no products or completed owner supplied;ZERO ROOT instructions,NOT positive qualification");return 0;
 }
 puts("START actual persistent3token flow/ONE ROOT+SAME119+ONE byte fabric/ONLY RAW learned+initial state+prompt data;no private-state transplant or CPU product upload");
 bool okay=m->RunTokens();if(m->kvFault){assert(!okay);m->VerifyKvFault();_exit(0);}
 assert(okay);m->VerifyTokens();
}
#endif
