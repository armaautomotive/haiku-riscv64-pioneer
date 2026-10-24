/* SPDX-License-Identifier: MIT */
// ACTUAL emitted fused worker pair. ONLY the NoC/GDDR fabric is simulated.
// Peer raw output is produced by the other actual QRJ1,never a host oracle.
#define PIONEER_Q38_GDN_RECURRENT_MACHINE_ONLY
#include "pioneer_q38_gdn_recurrent_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnGatedPlan.h"
struct GatedResidentMachine:GdnRecurrentMachine {
 std::array<uint32_t,64> gatedDescriptor={},gatedOutput={};
 std::array<uint32_t,128> norm={},z={};GatedResidentMachine* peer=nullptr;
 unsigned gatedPackets=0,readyPolls=0,phase=0,gatedWrites=0,readyWrites=0;
 unsigned gatedFail=0,gatedCounterFault=0,gatedForeignFault=0,gatedSourceFault=0,gatedDescriptorFault=0,gatedPoisonFault=0;
 unsigned injectedPhase=0;
 unsigned boundarySourceFault=0,predecessorDescriptorFault=0;bool boundaryInjected=false;
 unsigned peerStampFault=UINT32_MAX,peerRawFault=UINT32_MAX;bool neverReady=false;
 bool peerTerminalAtRawRead=false;
 GatedResidentMachine(const char* path,const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned half,unsigned bank,unsigned trial=1)
  :GdnRecurrentMachine(path,canonical,layer,half,bank,trial){
  PioneerQ38GdnGatedPlan plan;assert(plan.Build(canonical,inputDescriptor.data(),descriptor.data()));
  assert(plan.Matches(canonical,inputDescriptor.data(),descriptor.data()) && !plan.Build(canonical,inputDescriptor.data(),descriptor.data()));
  for(unsigned i=0;i<64;i++)gatedDescriptor[i]=plan.Descriptor()[i];
  SetGatedDescriptor();
  for(unsigned i=0;i<32;i++)memory[TT_Q38_GDN_GATED_SUMMARY+i*4]=0xdeadbeef;
  for(unsigned i=0;i<16;i++){memory[TT_Q38_GDN_GATED_READY+i*4]=0;memory[TT_Q38_GDN_GATED_PEER_READY+i*4]=0xdeadbeef;}
  for(unsigned i=0;i<128;i++){norm[i]=Bits(float(.5+sin(i*.07)*.3));z[i]=Bits(float(cos((half/2*128+i)*.017+trial*.11)*5));}
  for(unsigned i=0;i<64;i++)memory[0x8ea00+i*4]=gatedOutput[i]=0xdeadbeef;
  // Fresh scratch now spans the full raw128/norm128/Z128,not old rawQKV.
  for(unsigned i=324;i<384;i++)memory[0x80000+i*4]=0xdeadbeef;
  for(unsigned at:{0x74980U,0x749fcU,0x74b80U,0x74bfcU,0x74c80U,0x7fffcU,0x80600U,0x8e900U,0x8e9fcU,0x8eb00U})memory[at]=0xdecafbad;
 }
 void SetGatedDescriptor(){for(unsigned i=0;i<64;i++)memory[TT_Q38_GDN_GATED_DESCRIPTOR+i*4]=gatedDescriptor[i];}
 bool Terminal()const{return pc==0x64038 || pc==0x64058;}
 bool Passed()const{return pc==0x64038 && memory.at(0x17ffc4)==64;}
 uint32_t Load(uint32_t at)override{
  if(boundarySourceFault && !boundaryInjected && at==0xffb20208 && packets==11 && !readyWrites
   && memory.at(TT_Q38_RGDN_SUMMARY+4*4)==64){
   boundaryInjected=true;if(boundarySourceFault==1)seals[11]^=1;else memory[TT_Q38_GDN_GATED_READY+15*4]=1;
  }
  if(Range(at,TT_Q38_GDN_GATED_DESCRIPTOR,256) || Range(at,TT_Q38_GDN_GATED_SUMMARY,128)
   || Range(at,TT_Q38_GDN_GATED_READY,128) || Range(at,0x80000,1536) || Range(at,0x8e800,256) || Range(at,0x8ea00,256))return memory.at(at);
  if(!CodeAddress(at) && at<0xffb20000 && !Range(at,0x6e000,8192) && !Range(at,0x90000,32768)
   && !Range(at,0x74000,16) && !Range(at,TT_Q38_GDN_INPUT_DESCRIPTOR,384) && !Range(at,TT_Q38_RGDN_DESCRIPTOR,384)
   && !Range(at,0x74600,128) && !Range(at,0x80800,256) && !Range(at,uint32_t(L::Query),1288) && !Range(at,0x17ffc0,12))
   fprintf(stderr,"QGH1 unexpected load%08x PC%08x packets%u phase%u\n",at,pc,packets,phase);
  return GdnRecurrentMachine::Load(at);
 }
 virtual uint32_t GatedPayload(unsigned p,unsigned i){
  if(!p){
   assert(peer);uint32_t value=neverReady?0:peer->memory.at(TT_Q38_GDN_GATED_READY+i*4);
   if(i==peerStampFault && peer->memory.at(TT_Q38_GDN_GATED_READY)==TT_Q38_GDN_GATED_MARKER)value^=1;return value;
  }
  if(p==1){assert(peer && peer->memory.at(TT_Q38_GDN_GATED_READY)==TT_Q38_GDN_GATED_MARKER);peerTerminalAtRawRead|=peer->Passed();
   return i==peerRawFault?0x7fc00000:peer->memory.at(0x8e800+i*4);}
  return p==2?norm[i]:z[i];
 }
 void SubmitGated(){
  assert(peer && packets>=11 && nextWrites==8192 && resultWrites==64 && unit.drained);
  const auto& d=gatedDescriptor;unsigned p=phase,size[]={64,256,512,512,256};bool write=p==4;
  unsigned remote[]={d[44],d[45],d[12],d[4]+d[42],d[4]+d[43]};
  unsigned local[]={TT_Q38_GDN_GATED_PEER_READY,0x80000+((d[7]^1)&1)*256,0x80200,0x80400,0x8ea00};
  unsigned route[]={d[15],d[15],d[13],d[8],d[8]};
  const unsigned expected[]={write?local[p]:remote[p],0,write?d[9]:route[p],write?remote[p]:local[p],0,write?route[p]:d[9],0,write?0x212U:0,size[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  for(unsigned i=0;i<16;i++)assert(memory.at(TT_Q38_GDN_GATED_READY+i*4)==PioneerQ38GdnGatedReadyWord(d.data(),i,false));
  if(!write)for(unsigned i=0;i<size[p]/4;i++)assert(memory.at(local[p]+i*4)==0x7fc70000+p*1024+i);
  else assert(gatedWrites==64);
  packets++;gatedPackets++;
  if(gatedFail==p+1){injectedPhase=p+1;pending=true;return;}
  for(unsigned i=0;i<size[p]/4;i++){
   if(write)gatedOutput[i]=memory.at(local[p]+i*4);
   else if(gatedPoisonFault!=p+1 || i!=size[p]/4-1)memory[local[p]+i*4]=GatedPayload(p,i);
  }
  unsigned base=write?4:0;counts[base]++;counts[base+1]+=size[p]/64;counts[2]++;counts[6]=counts[2];counts[base+3]++;
  if(gatedPoisonFault==p+1)injectedPhase=p+1;
  if(gatedCounterFault==p+1){counts[base+3]++;injectedPhase=p+1;}
  if(gatedForeignFault==p+1){counts[write?0:4]++;injectedPhase=p+1;}
  if(gatedSourceFault==p+1){seals[11]^=1;injectedPhase=p+1;}
  if(gatedDescriptorFault==p+1){memory[TT_Q38_GDN_GATED_DESCRIPTOR+40*4]^=1;injectedPhase=p+1;}
  if(predecessorDescriptorFault && p==3)memory[(predecessorDescriptorFault==1?TT_Q38_GDN_INPUT_DESCRIPTOR:TT_Q38_RGDN_DESCRIPTOR)+40*4]^=1;
  if(!p){readyPolls++;if(memory.at(TT_Q38_GDN_GATED_PEER_READY))phase=1;}else phase++;
 }
 void Store(uint32_t at,uint32_t value)override{
  if(at==0xffb20040 && packets>=11){assert(value==1 && !pending);SubmitGated();return;}
  if(Range(at,TT_Q38_GDN_GATED_SUMMARY,128) || Range(at,TT_Q38_GDN_GATED_READY,128)
   || Range(at,0x80000,1536) || Range(at,0x8ea00,256)){
   if(Range(at,TT_Q38_GDN_GATED_READY,64)){
    assert(packets==11 && nextWrites==8192 && resultWrites==64);
    readyWrites++;if(at==TT_Q38_GDN_GATED_READY)assert(readyWrites==16 && value==TT_Q38_GDN_GATED_MARKER);
   }
   if(Range(at,0x8ea00,256)){assert(phase==4 && packets>=15 && !pending);gatedWrites++;}
   memory[at]=value;return;
  }
  GdnRecurrentMachine::Store(at,value);
 }
 void VerifyGated(){
  assert(Passed() && packets==11+readyPolls+4 && phase==5 && readyPolls && readyPolls<=256 && gatedWrites==64 && readyWrites==16);
  assert(memory.at(0x17ffc8)==TT_Q38_GDN_GATED_MARKER && returnWrites==3);
  DeltaVectorImage::Verify(); // ALL actual state/raw bits independently observed AFTER terminal
  for(unsigned i=0;i<8192;i++)assert(prior[i]==memory.at(0x90000+i*4) && next[i]==memory.at(0xa0000+i*4));
  uint32_t prep[32],step[32],gated[32];
  for(unsigned i=0;i<32;i++){prep[i]=memory.at(TT_Q38_GDN_INPUT_SUMMARY+i*4);step[i]=memory.at(TT_Q38_RGDN_SUMMARY+i*4);gated[i]=memory.at(TT_Q38_GDN_GATED_SUMMARY+i*4);}
  assert(PioneerQ38GdnInputSummary(prep,inputDescriptor.data(),initialCounts,step+8)
   && PioneerQ38ResidentGdnSummary(step,descriptor.data(),step+8,step+16)
   && PioneerQ38GdnGatedSummary(gated,gatedDescriptor.data(),inputDescriptor.data(),descriptor.data(),step+16,counts));
  for(unsigned i=0;i<32;i++){gated[i]^=1;assert(!PioneerQ38GdnGatedSummary(gated,gatedDescriptor.data(),inputDescriptor.data(),descriptor.data(),step+16,counts));gated[i]^=1;}
  float raw[128],sum=0;
  for(unsigned i=0;i<128;i++){
   auto* m=i/64==(descriptor[9]&1)?this:peer;raw[i]=Float(m->memory.at(0x8e800+(i%64)*4));sum+=raw[i]*raw[i];
   assert(memory.at(0x80000+i*4)==Bits(raw[i]) && memory.at(0x80200+i*4)==norm[i] && memory.at(0x80400+i*4)==z[i]);
  }
  float inverse=1.f/sqrtf(sum/128+1e-6f);
  for(unsigned i=0;i<64;i++){
   unsigned at=(descriptor[9]&1)*64+i;float weighted=(raw[at]*inverse)*Float(norm[at]);
   double expected=double(weighted)*double(Float(z[at]))/(1+exp(-double(Float(z[at])))),actual=Float(gatedOutput[i]);
   assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected)) && gatedOutput[i]==memory.at(0x8ea00+i*4));
   if(fabs(expected)>=double(Float(0x00800000)))assert(fabs(actual-expected)<=3e-6*fabs(expected));
  }
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_GDN_GATED_DESCRIPTOR+i*4)==gatedDescriptor[i]
   && memory.at(TT_Q38_GDN_INPUT_DESCRIPTOR+i*4)==inputDescriptor[i] && memory.at(TT_Q38_RGDN_DESCRIPTOR+i*4)==descriptor[i]);
  for(unsigned at:{0x74980U,0x749fcU,0x74b80U,0x74bfcU,0x74c80U,0x7fffcU,0x80600U,0x8e900U,0x8e9fcU,0x8eb00U})assert(memory.at(at)==0xdecafbad);
 }
};
static void RunGatedPair(GatedResidentMachine& a,GatedResidentMachine& b,unsigned first=0,unsigned skew=0){
 a.peer=&b;b.peer=&a;
 // Cooperative stepping executes BOTH original instruction streams. No
 // readiness/raw result is fabricated by the observer or copied by host.
 GatedResidentMachine* m[]={&a,&b};unsigned rounds=0;
 while(!a.Terminal() || !b.Terminal()){
  assert(++rounds<1000000);
  for(unsigned j=0;j<2;j++){
   auto* one=m[(j+first)%2];unsigned quantum=128+(j?skew:0);
   for(unsigned i=0;i<quantum && !one->Terminal();i++)one->StepCombined();
  }
 }
 for(auto* one:m){
  unsigned packets=one->packets,reads=one->nocReads,writes=one->commandWrites,ops=one->unit.ops;
  for(unsigned i=0;i<32;i++)one->StepCombined();
  assert(one->packets==packets && one->nocReads==reads && one->commandWrites==writes && one->unit.ops==ops);
  assert(one->memory.at(0x17ffc8)==(one->pc==0x64058?TT_Q38_GDN_GATED_FAULT:TT_Q38_GDN_GATED_MARKER));
 }
}
#ifndef PIONEER_Q38_GDN_GATED_RESIDENT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);(void)&TestWorker;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned positives=0,negatives=0,maxSteps=0,maxStack=0,maxPolls=0;
 for(unsigned head=0;head<48;head++){
  unsigned layer=(head/3)*4+head%3;
  GatedResidentMachine a(argv[1],*canonical,layer,head*2,head%7,1+head%4),b(argv[1],*canonical,layer,head*2+1,head%7,1+head%4);
  RunGatedPair(a,b,head%2,head%3*32);a.VerifyGated();b.VerifyGated();positives+=2;
  for(auto* m:{&a,&b}){maxSteps=std::max(maxSteps,m->steps);maxStack=std::max(maxStack,0x70000-m->minStack);maxPolls=std::max(maxPolls,m->readyPolls);}
 }
 printf("PASS ACTUAL QGH1 paired fused workers%u/all48heads/7banks/bothslots/SYNTHETIC numerics;rawQKV->prep->SFPU NEXT->fresh peer-ready NoC->paired raw128+learned norm+retainedZ->gated64 GDDR;NO host intermediate;ALL state/raw bits+6144gatedvalues+strict NORMAL relative gates;maxSteps%u/maxStack%u/maxReadyPolls%u;OFFLINE ONLY\n",positives,maxSteps,maxStack,maxPolls);
 {
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);a.peer=&b;b.peer=&a;
  while(a.readyWrites!=16 || b.readyWrites!=16){
   if(a.readyWrites!=16)a.StepCombined();if(b.readyWrites!=16)b.StepCombined();
  }
  while(!b.Terminal())b.StepCombined();assert(b.Passed());
  while(!a.Terminal())a.StepCombined();assert(a.Passed() && a.peerTerminalAtRawRead);
  a.VerifyGated();b.VerifyGated();positives+=2;
  puts("PASS ACTUAL QGH1 lagging half reads peer raw AFTER peer final terminal;separate raw-ready seal retained,not reused final marker");
 }
 {
  GatedResidentMachine a(argv[1],*canonical,0,0,0);PioneerQ38GdnGatedPlan plan;
  assert(plan.Build(*canonical,a.inputDescriptor.data(),a.descriptor.data()));
  std::vector<uint8_t> pages(PioneerResidentBanks::Pages,0);
  const auto* d=plan.Descriptor();unsigned at=d[11]*unsigned(canonical->Capacity/4096)+(d[12]-canonical->Base)/4096;
  assert(!plan.LearnedPagePresent(pages.data(),unsigned(pages.size())));
  pages[at]=2;assert(!plan.LearnedPagePresent(pages.data(),unsigned(pages.size())));
  pages[at]=1;assert(plan.LearnedPagePresent(pages.data(),unsigned(pages.size()))
   && !plan.LearnedPagePresent(nullptr,unsigned(pages.size())) && !plan.LearnedPagePresent(pages.data(),unsigned(pages.size())-1));
  auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build()
   && !plan.Matches(*other,a.inputDescriptor.data(),a.descriptor.data()));
  a.descriptor[8]++;assert(!plan.Matches(*canonical,a.inputDescriptor.data(),a.descriptor.data()));
  puts("PASS QGH1 DATA plan exact original role9 norm512/page4096/missing-corrupt-upload-ledger/refused wrong canonical owner/changed epoch;NO native uploaded/source grant");
 }
 unsigned edgeHalves=0,edgeMaxSteps=0;
 for(float gate:{0.f,-80.f,-81.f,-84.f,-87.f,-88.f,-90.f,-100.f,-104.f,-106.f,-128.f,-65504.f,65504.f}){
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);
  for(auto* m:{&a,&b})for(unsigned i=0;i<128;i++){m->z[i]=Bits(gate);m->norm[i]=Bits((i&1)?65504.f:-65504.f);}
  RunGatedPair(a,b);a.VerifyGated();b.VerifyGated();edgeHalves+=2;
  edgeMaxSteps=std::max(edgeMaxSteps,std::max(a.steps,b.steps));
 }
 printf("PASS ACTUAL fused QGH1 edge halves%u:zero/negative80..106/128/65504/positive65504 gates,max signed learned weights,strict NORMAL final relative3e-6;maxSteps%u;NOT speed\n",edgeHalves,edgeMaxSteps);
 for(unsigned head:{0U,47U}){
  std::array<uint32_t,8192> carry[2]={};uint32_t counters[2][8];for(auto& side:counters)for(auto& value:side)value=0xfffffff8;
  for(unsigned token=0;token<16;token++){
   GatedResidentMachine a(argv[1],*canonical,0,head*2,token%7,token+1),b(argv[1],*canonical,0,head*2+1,token%7,token+1);
   unsigned side=0;for(auto* m:{&a,&b}){
    m->prior=carry[side];m->inputDescriptor[6]=m->descriptor[8]=m->gatedDescriptor[6]=17+token;
    m->inputDescriptor[40]=m->descriptor[40]=m->gatedDescriptor[40]=token;m->SetDescriptors();m->SetGatedDescriptor();
    for(unsigned i=0;i<8;i++)m->counts[i]=m->initialCounts[i]=counters[side][i];side++;
   }
   RunGatedPair(a,b,token%2,32);a.VerifyGated();b.VerifyGated();side=0;
   for(auto* m:{&a,&b}){carry[side]=m->next;for(unsigned i=0;i<8;i++)counters[side][i]=m->counts[i];side++;}
  }
 }
 puts("PASS ACTUAL fused QGH1 two heads x16 NONRESETTING state+NoC-counter chains/64half streams/bothslots/monotonic phase+position/fresh per-run seals;component ONLY,NOT persistent native/model-token scheduler");
 // Actual stale-ready refusal happens BEFORE ANY predecessor NoC/SFPU.
 for(unsigned word=0;word<16;word++){
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);a.memory[TT_Q38_GDN_GATED_READY+word*4]=1;
  RunGatedPair(a,b);assert(!a.Passed() && !a.packets && !a.nocReads && !a.unit.ops && !b.Passed() && !b.gatedWrites);negatives++;
 }
 for(unsigned word=0;word<64;word++)for(unsigned bit=0;bit<32;bit++){
  GatedResidentMachine a(argv[1],*canonical,0,0,0);a.gatedDescriptor[word]^=1U<<bit;
  if(PioneerQ38GdnGatedShape(a.gatedDescriptor.data(),a.inputDescriptor.data(),a.descriptor.data()))continue;
  a.SetGatedDescriptor();while(!a.Terminal())a.StepCombined();assert(!a.Passed() && !a.packets && !a.nocReads && !a.unit.ops);negatives++;
 }
 for(unsigned word=0;word<16;word++){
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);a.peerStampFault=word;
  RunGatedPair(a,b);assert(!a.Passed() && !a.gatedWrites && a.phase==1);negatives++;
 }
 for(unsigned kind=0;kind<6;kind++)for(unsigned packet=1;packet<=5;packet++){
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);
  if(!kind)a.gatedFail=packet;if(kind==1)a.gatedCounterFault=packet;if(kind==2)a.gatedForeignFault=packet;
  if(kind==3)a.gatedSourceFault=packet;if(kind==4)a.gatedDescriptorFault=packet;if(kind==5)a.gatedPoisonFault=packet;
  if(kind==5 && packet==5)continue; // write has no poisoned read destination
  printf("QGH1 fault kind%u phase%u\n",kind,packet);
  RunGatedPair(a,b,0,0);assert(!a.Passed() && a.injectedPhase==packet);negatives++;
 }
 for(unsigned word:{0U,63U}){
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);a.peerRawFault=word;
  RunGatedPair(a,b);assert(!a.Passed() && !a.gatedWrites && a.phase==2);negatives++;
 }
 for(unsigned kind=1;kind<=2;kind++){
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);a.boundarySourceFault=kind;
  RunGatedPair(a,b);assert(!a.Passed() && a.boundaryInjected && !a.readyWrites && !a.gatedPackets && !b.Passed());negatives++;
 }
 for(unsigned side=1;side<=2;side++){
  GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);a.predecessorDescriptorFault=side;
  RunGatedPair(a,b);assert(!a.Passed() && a.phase==4 && a.gatedWrites==64
   && !a.memory.at(TT_Q38_GDN_GATED_SUMMARY+4*4));negatives++;
 }
 {GatedResidentMachine a(argv[1],*canonical,0,0,0),b(argv[1],*canonical,0,1,0);a.neverReady=true;
  RunGatedPair(a,b);assert(!a.Passed() && a.readyPolls==256 && !a.gatedWrites);negatives++;}
 printf("PASS ACTUAL QGH1 negatives%u:ALL descriptor bits/stale own16words/wrong peer16fields/256bounded poll timeout/NoC timeout-counter-foreign-source-descriptor-poison/peer NaN;terminal parks inert;NO native/source/ready/producer/current-slot authority or hardware speed claim\n",negatives);
}
#endif
