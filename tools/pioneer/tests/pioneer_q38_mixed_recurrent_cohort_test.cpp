/* SPDX-License-Identifier: MIT */
// Actual NEW ROOT + actual SAME119 QMW1 peers. Only byte/register/clock
// endpoints are modeled. No success status,products,nonce or PC is seeded.
#define PIONEER_Q38_MIXED_WORKER_MACHINE_ONLY
#include "pioneer_q38_mixed_worker_test.cpp"
#define PIONEER_GRAPH_MATH_BASE_ALREADY_INCLUDED
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../PioneerQ38MixedRecurrentCohortPlan.h"
#include <sys/wait.h>
#include <unistd.h>
namespace RC=PioneerQ38MixedRecurrentCohortLayout;
struct CohortBytes {
 virtual ~CohortBytes()=default;
 virtual uint32_t Get(unsigned route,unsigned address)const=0;
 // Byte endpoint batching ONLY. Every worker instruction and packet still
 // executes. The default is exactly the previous little-endian Get loop.
 virtual void ReadRaw(unsigned route,unsigned address,uint8_t* out,unsigned bytes)const{
  assert(out && bytes && !((address|bytes)&3) && uint64_t(address)+bytes<=UINT64_C(0x100000000));
  for(unsigned i=0;i<bytes;i+=4){uint32_t word=Get(route,address+i);for(unsigned b=0;b<4;b++)out[i+b]=uint8_t(word>>(b*8));}
 }
 virtual void Put(unsigned route,unsigned address,uint32_t value)=0;
 virtual uint64_t Clock()const=0;
 virtual uint32_t RootWord(unsigned address)const=0;
};
struct NewCohortPeer:MixedPeer {
 CohortBytes* bytes=nullptr;bool active=false;
 using MixedPeer::MixedPeer;
 void Received(){
  assert(!active && (pc==waitPC || pc==waitPC+4));ClearTensorState();
  unsigned next=memory.at(M::Job+4),op=memory.at(M::Job+8);assert(next==sequence+1 && (op==1 || op==2 || op==7 || op==8));
  if(op==2){sequence=next;active=true;return;} // execute the actual STOP;no tensor or counter rewrite
  if(op==1){MatrixReceived();return;}
  uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  sequence=next;denseActive=outputActive=false;activation=true;convActive=op==7;gdnActive=op==8;
  for(unsigned i=0;i<8;i++)countBase[i]=physical[i];
  if(op==7){
   convPackets=convWrites=0;for(unsigned i=0;i<384;i++)table[i]=memory.at(TT_Q38_CONV_BATCH_TABLE+i*4);
  }else{
   gdnPackets=gdnPhase=gdnPolls=gdnImports=gdnOps=clockReads=0;
   gdnNextWrites=gdnOutputWrites=gdnGatedWrites=gdnCsrReads=0;
   for(unsigned i=0;i<64;i++){
    input[i]=memory.at(TT_Q38_GDN_INPUT_DESCRIPTOR+i*4);state[i]=memory.at(TT_Q38_RGDN_DESCRIPTOR+i*4);
    gated[i]=memory.at(TT_Q38_GDN_GATED_DESCRIPTOR+i*4);
   }
   for(unsigned i=0;i<8;i++)gdnBefore[i]=physical[i];
  }
  active=true;
 }
 void MatrixReceived(){
  ClearTensorState();uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  sequence=memory.at(M::Job+4);denseActive=gdnActive=convActive=outputActive=activation=false;
  type=memory.at(M::Job+17*4);inputs=memory.at(M::Job+21*4);FfnPersistentPeer::groups=memory.at(M::Job+23*4);
  base=memory.at(M::Job+22*4);unsigned route=memory.at(M::Job+24*4);count=type==8?32:256;
  assert((type==8 || type==12) && FfnPersistentPeer::groups && FfnPersistentPeer::groups<=16);
  for(unsigned i=0;i<8;i++)countBase[i]=physical[i];requests=responses=matrixInstructions=0;doorbellRead=false;
  tile.assign(PioneerQ38PanelTileBytes(type),0);tiles.assign(FfnPersistentPeer::groups*(inputs/count),tile);vector.resize(inputs);scales.resize(inputs/count);
  packets.clear();root.clear();unsigned scaleBytes=(unsigned(scales.size()*4)+63)&~63U;
  for(unsigned at=0;at<inputs*2;at+=16384)packets.push_back({0xa0000+at,129,0xa0000+at,std::min(16384U,inputs*2-at),-1});
  packets.push_back({0xab000,129,0x94000,scaleBytes,-1});inputReads=unsigned(packets.size());
  for(unsigned i=0;i<inputs/2;i++)root[0xa0000+i*4]=bytes->RootWord(0xa0000+i*4);
  for(unsigned i=0;i<scaleBytes/4;i++)root[0xab000+i*4]=bytes->RootWord(0xab000+i*4);
  for(unsigned i=0;i<inputs;i++)vector[i]=uint16_t(root.at(0xa0000+i/2*4)>>((i%2)*16));
  for(unsigned i=0;i<scales.size();i++)scales[i]=root.at(0xab000+i*4);
  for(unsigned t=0;t<tiles.size();t++){
   bytes->ReadRaw(route,base+t*unsigned(tile.size()),tiles[t].data(),unsigned(tile.size()));
   packets.push_back({base+t*unsigned(tile.size()),route,0x90000,unsigned(tile.size()),int(t)});
  }
  // These poison only scratch/cache observer sentinels, never PC/registers,
  // private nonce, configuration, LRegs or physical tensor accumulators.
  Poison();PoisonTail();for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;active=true;
 }
 void SubmitConvBytes(){
  ClearTensorState();unsigned n=PioneerQ38ConvBatchCount(logicalWorker),j=convPackets/5,p=convPackets%5;assert(j<n);convPackets++;
  bool write=p>=3;const auto* d=table.data()+j*64;
  const unsigned local[]={0x80000,0x80400,0x80800,0x80c00,0x81000},size[]={64,256,192,192,64};
  const unsigned remote[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},route[]={d[8],d[13],d[41],d[8],d[8]};
  const uint32_t expected[]={write?local[p]:remote[p],0,write?selfRoute:route[p],write?remote[p]:local[p],0,
   write?route[p]:selfRoute,0,write?0x212U:0,size[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  for(unsigned i=0;i<size[p]/4;i++)if(write){bytes->Put(route[p],remote[p]+i*4,memory.at(local[p]+i*4));convWrites++;}
  else {assert(memory.at(local[p]+i*4)==0x7fc50000+p*1024+i);memory[local[p]+i*4]=bytes->Get(route[p],remote[p]+i*4);}
  unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=size[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
 }
 void SubmitGdnBytes(){
  unsigned p=gdnPackets;
  if(p<7){
   const unsigned remote[]={input[4]+input[42],input[4]+input[43],input[4]+input[44],input[4]+input[45],input[4]+input[46],input[12],input[15]};
   const unsigned route[]={input[8],input[8],input[8],input[8],input[8],input[13],input[41]},words[]={128,128,64,16,16,16,16};
   for(unsigned i=0;i<words[p];i++){uint32_t value=bytes->Get(route[p],remote[p]+i*4);if(p<3)raw[(p==0?0:p==1?128:256)+i]=value;else learned[p-3][i]=value;}
   if(p>=3)raw[320+p-3]=learned[p-3][input[47]];
  }
  if(p==7 || p==8)for(unsigned i=0;i<4096;i++)prior[(p-7)*4096+i]=bytes->Get(state[12],state[5]+((p-7)*4096+i)*4);
  if(p>=11 && gdnPhase==2)for(unsigned i=0;i<128;i++)norm[i]=bytes->Get(gated[13],gated[12]+i*4);
  if(p>=11 && gdnPhase==3)for(unsigned i=0;i<128;i++)z[i]=bytes->Get(gated[8],gated[4]+gated[42]+i*4);
  ResidentGdnOperatorPeer::SubmitGdn();
  if(p==9 || p==10)for(unsigned i=0;i<4096;i++)bytes->Put(state[13],state[7]+((p-9)*4096+i)*4,next[(p-9)*4096+i]);
  if(gdnPhase==5)for(unsigned i=0;i<64;i++)bytes->Put(gated[8],gated[4]+gated[43]+i*4,gatedOutput[i]);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(a==0xffb20040 && active && (convActive || gdnActive)){
   assert(value==1 && !pending);if(gdnActive)SubmitGdnBytes();else SubmitConvBytes();return;
  }
  MixedPeer::Store(a,value);
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(bytes && gdnActive && (csr==0xb00 || csr==0xb80));clockReads++;uint64_t now=bytes->Clock();return csr==0xb00?uint32_t(now):uint32_t(now>>32);
 }
};
struct NewCohortMachine:ScalarMachine,CohortBytes {
 std::vector<uint8_t> bootRom,text,convRom;
 std::array<std::unique_ptr<NewCohortPeer>,119> child;
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38MixedRecurrentCohortPlan> plan=std::make_unique<PioneerQ38MixedRecurrentCohortPlan>();
 std::array<uint32_t,64> boot={},header={};std::array<uint32_t,16> source={};std::array<uint32_t,10> command={};
 std::array<uint32_t,8> counts={};std::array<unsigned,119> initial={};std::array<bool,96> cleared={},verified={};
 std::map<uint64_t,uint32_t> dram;uint64_t cycles=UINT64_C(1)<<40;
 unsigned packets=0,rootConvPackets=0,convStarts=0,gdnStarts=0,failPacket=0,counterFaultPacket=0,sourceFaultPacket=0;
 unsigned commandFaultPacket=0,badResetWorker=0,badProofWorker=0,badProofWord=0,badProofStage=0,dropClearWorker=0,heldPcReads=0;
 unsigned failRootConv=0,failGdnRelease=0,mutateConvOnWrite=0,peerWrites=0;
 bool pending=false,unknown=false,faultHit=false;
 static uint64_t Key(unsigned route,unsigned address){return uint64_t(route)<<32|address;}
 uint32_t Get(unsigned route,unsigned address)const override{return dram.at(Key(route,address));}
 void Put(unsigned route,unsigned address,uint32_t v)override{dram[Key(route,address)]=v;}
 uint64_t Clock()const override{return cycles;}
 uint32_t RootWord(unsigned address)const override{return memory.at(address);}
 NewCohortMachine(const char* image,const char* worker,bool seedPrimitiveOutputs=true,bool virgin=false):ScalarMachine(0,0){
  assert(canonical->Build());bootRom=Read((std::string(image)+"/boot.bin").c_str());text=Read((std::string(image)+"/text.bin").c_str());
  convRom=Read((std::string(image)+"/conv.bin").c_str());l1[0]=Word(bootRom,0);counts.fill(0xfffffff8);
  std::array<uint32_t,1920> sources;
  for(unsigned i=0;i<16;i++)sources[i]=source[i]=i?0x76540000+i:0x16005;
  for(unsigned w=0;w<119;w++){
   child[w]=std::make_unique<NewCohortPeer>(worker,w+1,8,5120);auto& p=*child[w];p.bytes=this;
   // Distinct physical source registers and boot bytes BEFORE first boot.
   for(unsigned i=0;i<16;i++)p.boot[16+i]=sources[(w+1)*16+i]=p.source[i]+=w*0x100;
   p.boot[5]=p.source[0];for(unsigned i=0;i<64;i++)p.memory[M::Boot+i*4]=p.boot[i];
   assert(p.BootMixed());if(!virgin)for(unsigned job=0;job<1+w%3;job++){if(job)FreshInputs(p);assert(p.MatrixMixed());}
   initial[w]=p.sequence;
  }
  assert(plan->Build(*canonical,0,0,0,0,0,1001,1,{1,2,3,4},sources.data()));
  for(unsigned i=0;i<64;i++)header[i]=plan->Header()[i];
  boot[0]=RC::Magic;boot[1]=1;boot[2]=64;boot[3]=129;boot[4]=source[0];boot[5]=119;boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;for(unsigned i=0;i<16;i++)boot[16+i]=source[i];assert(RC::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++){memory[RC::Boot+i*4]=boot[i];memory[RC::Header+i*4]=header[i];memory[RC::Summary+i*4]=0x7fc00001;}
  memory[RC::Doorbell]=0;for(unsigned i=0;i<16;i++)memory[RC::Status+i*4]=0x7fc00001;
  for(auto r:std::array<std::pair<unsigned,unsigned>,10>{{{RC::Conv,RC::ConvWords*4},{RC::ConvShadow,RC::ConvWords*4},
   {RC::Gdn,RC::GdnWords*4},{RC::GdnShadow,RC::GdnWords*4},{RC::Counters,119*32},{RC::Nonces,119*4},
   {RC::PeerBoots,119*256},{RC::PeerJobs,119*256},{RC::Scratch,16384},{0x6e000,8192}}})
   for(unsigned a=r.first;a<r.first+r.second;a+=4)memory[a]=0x7fc00001;
  for(unsigned i=0;i<RC::ConvWords;i++)memory[RC::Conv+i*4]=plan->Conv()[i];
  for(unsigned i=0;i<RC::GdnWords;i++)memory[RC::Gdn+i*4]=plan->Gdn()[i];
  for(unsigned a=0x74200;a<0x74e00;a+=4)if(!Range(a,RC::Header,256))memory[a]=0x7fc00001;
  if(seedPrimitiveOutputs)Seed();for(unsigned w=0;w<96;w++)child[w]->peer=child[w^1].get();
 }
 void Seed(){
  unsigned base=header[8],route=header[9];
  for(unsigned i=0;i<6144;i++)Put(route,base+PioneerQ38ProjectionResultOffset(5)+i*4,Bits(float(cos(i*.017+.11))*5));
  for(unsigned i=0;i<10240;i++)Put(route,base+PioneerQ38ProjectionResultOffset(6)+i*4,Bits(float(sin(i*.017+.231))));
  for(unsigned h=0;h<48;h++){Put(route,base+PioneerQ38ProjectionResultOffset(10)+h*4,Bits(float(h)*.13f-2));Put(route,base+PioneerQ38ProjectionResultOffset(11)+h*4,Bits(float(h)*.07f-2));}
  for(unsigned w=0;w<120;w++)for(unsigned job=0;job<PioneerQ38ConvBatchCount(w);job++){
   const auto* d=plan->Conv()+w*384+job*64;
   for(unsigned c=0;c<16;c++){
    for(unsigned t=0;t<4;t++)Put(d[13],d[12]+(c*4+t)*4,Bits(float(int(((d[7]+c)*7+t*11)%29)-14)*.03125f));
    for(unsigned t=0;t<3;t++)Put(d[41],d[15]+(c*3+t)*4,Bits(float(cos((d[7]+c)*.013+t*.117+.031))));
    Put(d[8],d[4]+d[43]+c*4,0x7fc00000);
   }
  }
  for(unsigned w=0;w<96;w++){
   const auto* in=plan->Gdn()+w*192;const auto* st=in+64;const auto* ga=in+128;
   for(unsigned i=0;i<16;i++){Put(in[13],in[12]+i*4,Bits(-float(i+1)*.25f));Put(in[41],in[15]+i*4,Bits(float(i+1)*.01f));}
   for(unsigned i=0;i<128;i++)Put(ga[13],ga[12]+i*4,Bits(float(.5+sin(i*.07)*.3)));
   for(unsigned i=0;i<8192;i++){Put(st[12],st[5]+i*4,Bits(float(sin((i+w*8192)*.003))*0.0001f));Put(st[13],st[7]+i*4,0x7fc00000);}
   for(unsigned i=0;i<16;i++)child[w]->memory[TT_Q38_GDN_GATED_READY+i*4]=0xfeed0000+w*16+i;
  }
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,RC::Code,unsigned(text.size())) || Range(a,RC::ConvCode,unsigned(convRom.size()));}
 unsigned Index(uint32_t a)override{
  assert(CodeAddress(a) && !(a&3));if(a==0x6000)return 0;
  l1[8180]=Range(a,RC::Code,unsigned(text.size()))?Word(text,a-RC::Code):Word(convRom,a-RC::ConvCode);return 8180;
 }
 unsigned StepLimit()const override{return 3000000000U;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  if(Range(a,0xffb20000,0x2000)){
   unsigned off=a-0xffb20000;if(off<40)return command[off/4];
   for(unsigned i=0;i<4;i++)if(off==0x40+i*0x800)return i?0:unsigned(pending);
   if(off>=0x240 && off<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(off==BlackholeDramSourceOffset(i))return source[i];
   for(unsigned i=0;i<8;i++)if(off==FfnPersistentPeer::CounterOffset(i))return counts[i];assert(false);
  }
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && !CodeAddress(a));
  if(Range(a,0xffb20000,40)){assert(!pending && !unknown);command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending && !unknown);Submit();return;}
  bool allowed=Range(a,0x6e000,8192) || Range(a,RC::Status,64) || Range(a,RC::Summary,256) || Range(a,RC::Scratch,16384)
   || Range(a,RC::ConvShadow,RC::ConvWords*4) || Range(a,RC::GdnShadow,RC::GdnWords*4)
   || Range(a,RC::Counters,119*32) || Range(a,RC::Nonces,119*4) || Range(a,RC::PeerBoots,119*256) || Range(a,RC::PeerJobs,119*256)
   || Range(a,TT_Q38_CONV_BATCH_TABLE,1536) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128) || Range(a,0x80000,0x1100);
  if(!allowed)fprintf(stderr,"UNMAPPED ROOT STORE address%08x value%08x PC%08x\n",a,value,pc);
  assert(allowed);if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  memory[a]=value;if(a==RC::Status+8 && value==RC::Fault)unknown=true;
 }
 void Tick(){
  for(auto& p:child)if(p->active)for(unsigned i=0;i<64;i++){
   if(p->GdnFaulted() || p->GdnFinished(p->sequence) || p->pc==p->stopPC){p->active=false;break;}
   cycles+=19;p->StepBlock();
  }
 }
 void Step(){cycles+=19;StepScalar();if(!(steps&255))Tick();}
 virtual void Submit(){
  packets++;if(packets==failPacket){pending=true;return;}
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],remote=write?command[3]:command[0],local=write?command[0]:command[3],bytes=command[8];
  assert(!command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
  if(Range(pc,RC::ConvCode,unsigned(convRom.size()))){
   if(failRootConv && !rootConvPackets){pending=faultHit=true;return;}
   assert(local>=0x80000 && local+bytes<=0x81100);for(unsigned i=0;i<bytes/4;i++)if(write)Put(route,remote+i*4,memory.at(local+i*4));else memory[local+i*4]=Get(route,remote+i*4);
   rootConvPackets++;
  }else{
   unsigned w=0;while(w<119 && M::Peer(w+1)!=route)w++;assert(w<119);auto& p=*child[w];
   assert(local==RC::Scratch+(remote&31) && bytes && bytes<=1536);
   if(write){
    assert(!p.active);
    if(remote==M::Doorbell && p.memory.at(M::Job+8)==8 && failGdnRelease){pending=faultHit=true;return;}
    peerWrites++;
    if(remote==TT_Q38_CONV_BATCH_TABLE && mutateConvOnWrite && !faultHit){memory[RC::Conv+RC::ConvWords*4-4]^=1;faultHit=true;}
    if(remote==TT_Q38_GDN_GATED_READY){assert(!gdnStarts && bytes==64);cleared[w]=true;}
    if(remote!=TT_Q38_GDN_GATED_READY || dropClearWorker!=w+1)for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
    if(remote==M::Doorbell){
     unsigned op=p.memory.at(M::Job+8);
     if(op==8){assert(convStarts==119 && rootConvPackets==30);for(unsigned i=0;i<96;i++)assert(cleared[i] && verified[i]);gdnStarts++;}else {assert(op==7 && !gdnStarts);convStarts++;}
     p.Received();
    }
   }else{
    if(remote==0xffb121b0){assert(bytes==4);memory[local]=badResetWorker==w+1?0x46801:0x46800;}
    else if(remote==0xffb13140){assert(bytes==4);if(badResetWorker==w+1)heldPcReads++;memory[local]=p.pc;}
    else if(Range(remote,0xffb20000,0x2000)){
     bool found=false;unsigned off=remote-0xffb20000;
     for(unsigned i=0;i<16;i++)if(off==BlackholeDramSourceOffset(i)){memory[local]=p.source[i];found=true;}
     for(unsigned i=0;i<8;i++)if(off==FfnPersistentPeer::CounterOffset(i)){memory[local]=p.Count(i);found=true;break;}assert(found);
    }else{
     for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
     unsigned stage=gdnStarts?2:convStarts?1:0;
     if(remote==0x83f00 && badProofWorker==w+1 && badProofStage==stage){memory[local+badProofWord*4]^=1;faultHit=true;}
     if(remote==TT_Q38_GDN_GATED_READY){assert(!gdnStarts && bytes==64);verified[w]=true;for(unsigned i=0;i<16;i++)verified[w] &= memory[local+i*4]==0;}
    }
   }
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=(bytes+63)>>6;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==counterFaultPacket)counts[3]++;if(packets==sourceFaultPacket)source[11]^=1;
  if(packets==commandFaultPacket)command[5]^=1;
 }
 bool Wait(unsigned n){
  while(pc!=0x4009c && !(pc==0x4004c && memory.at(RC::Status+4)==n))Step();
  return pc!=0x4009c;
 }
 void Stage(){memory[RC::Doorbell]=header[48];}
 void VerifyFault(){
  assert(!Wait(1) && unknown && memory.at(RC::Status+8)==RC::Fault && memory.at(RC::Status+4)!=1
   && memory.at(RC::Summary)!=RC::Magic && pc==0x4009c);
  // No fabricated STOP/cleanup. Even another host doorbell cannot resume a
  // ROOT with unknown ownership. In-flight peers are deliberately not reset.
  unsigned issued=packets,writes=peerWrites;auto commands=command;auto physical=counts;
  memory[RC::Doorbell]=2;for(unsigned i=0;i<2048;i++)StepScalar();
  assert(pc==0x4009c && packets==issued && peerWrites==writes && command==commands && counts==physical);
 }
 void Verify(){
  assert(!unknown && !pending && !heldPcReads && convStarts==119 && gdnStarts==96 && rootConvPackets==30
   && memory.at(RC::Summary)==RC::Magic && memory.at(RC::Summary+4)==1 && memory.at(RC::Summary+4*4)==120
   && memory.at(RC::Summary+5*4)==6 && memory.at(RC::Summary+6*4)==96 && memory.at(RC::Summary+7*4)==6144
   && memory.at(RC::Summary+8*4)==packets && minStack>=0x6e000);
  for(unsigned w=0;w<120;w++)for(unsigned job=0;job<PioneerQ38ConvBatchCount(w);job++)for(unsigned c=0;c<16;c++){
   const auto* d=plan->Conv()+w*384+job*64;float sum=0;
   for(unsigned t=0;t<4;t++)sum+=Float(t==3?Get(d[8],d[4]+d[45]+c*4):Get(d[41],d[15]+(c*3+t)*4))*Float(Get(d[13],d[12]+(c*4+t)*4));
   double expected=double(sum)/(1+exp(-double(sum))),actual=Float(Get(d[8],d[4]+d[43]+c*4));
   assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected)));
   for(unsigned t=0;t<3;t++)assert(Get(d[8],d[4]+d[42]+(c*3+t)*4)==(t==2?Get(d[8],d[4]+d[45]+c*4):Get(d[41],d[15]+(c*3+t+1)*4)));
  }
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];assert(!p.active && p.GdnFinished(p.sequence) && p.sequence==initial[w]+(w<96?2U:1U)
    && memory.at(RC::Nonces+w*4)==p.sequence);
   if(w<96){p.VerifyGdn(p.sequence);for(unsigned i=0;i<64;i++)assert(Get(p.gated[8],p.gated[4]+p.gated[43]+i*4)==p.gatedOutput[i]);}
  }
 }
};
#ifndef PIONEER_Q38_MIXED_RECURRENT_COHORT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 puts("START actual QRC1 ROOT+SAME119 QMW1/unequal real matrix histories/ROOT6Conv+119Conv->96GDN;synthetic primitive bytes,NOT full model");
 auto m=std::make_unique<NewCohortMachine>(argv[1],argv[2]);assert(m->Wait(0));
 unsigned cases=0;
 auto bad=[&](const char* name,auto setup,auto inspect){
  pid_t pid=fork();assert(pid>=0);
  if(!pid){setup(*m);m->Stage();m->VerifyFault();inspect(*m);printf("PASS refusal %s/ROOTpark/no later packet or reset\n",name);_exit(0);}
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
 };
 auto noIO=[](const NewCohortMachine& q){assert(!q.packets && !q.peerWrites && !q.convStarts && !q.gdnStarts);};
 auto noLaunch=[](const NewCohortMachine& q){assert(!q.convStarts && !q.gdnStarts);};
 bad("reserved header",[](NewCohortMachine& q){q.memory[RC::Header+52*4]=1;},noIO);
 bad("private ROOT boot changed",[](NewCohortMachine& q){q.memory[RC::Boot+20*4]^=1;},noIO);
 bad("last Conv table reserved word",[](NewCohortMachine& q){q.memory[RC::Conv+RC::ConvWords*4-4]^=1;},noIO);
 bad("last GDN table reserved word",[](NewCohortMachine& q){q.memory[RC::Gdn+RC::GdnWords*4-4]^=1;},noIO);
 for(unsigned w:{1U,119U})bad("held reset before PC",[=](NewCohortMachine& q){q.badResetWorker=w;},
  [=](const NewCohortMachine& q){assert(!q.heldPcReads && !q.peerWrites && !q.convStarts && !q.gdnStarts);});
 bad("last peer physical source changed",[](NewCohortMachine& q){q.child[118]->source[11]^=1;},noLaunch);
 bad("last predecessor proof changed",[](NewCohortMachine& q){q.badProofWorker=119;q.badProofWord=63;},
  [](const NewCohortMachine& q){assert(q.faultHit && !q.peerWrites && !q.convStarts && !q.gdnStarts);});
 bad("unknown first packet",[](NewCohortMachine& q){q.failPacket=1;},
  [](const NewCohortMachine& q){assert(q.pending && q.packets==1 && !q.peerWrites);});
 bad("ROOT counter discontinuity",[](NewCohortMachine& q){q.counterFaultPacket=1;},noLaunch);
 bad("ROOT physical source changed",[](NewCohortMachine& q){q.sourceFaultPacket=1;},noLaunch);
 bad("ROOT command readback changed",[](NewCohortMachine& q){q.commandFaultPacket=1;},noLaunch);
 bad("primary table changed after capture",[](NewCohortMachine& q){q.mutateConvOnWrite=1;},
  [](const NewCohortMachine& q){assert(q.faultHit && q.peerWrites==238 && !q.convStarts && !q.gdnStarts);});
 bad("unknown ROOT Conv packet",[](NewCohortMachine& q){q.failRootConv=1;},
  [](const NewCohortMachine& q){assert(q.faultHit && q.pending && q.convStarts==119 && !q.rootConvPackets && !q.gdnStarts);});
 bad("closed Conv proof changed",[](NewCohortMachine& q){q.badProofStage=1;q.badProofWorker=119;q.badProofWord=19;},
  [](const NewCohortMachine& q){assert(q.faultHit && q.convStarts==119 && !q.gdnStarts && q.rootConvPackets==30);});
 bad("last ready clear lost",[](NewCohortMachine& q){q.dropClearWorker=96;},
  [](const NewCohortMachine& q){assert(q.cleared[95] && !q.verified[95] && !q.gdnStarts && q.convStarts==119);});
 bad("unknown first GDN release",[](NewCohortMachine& q){q.failGdnRelease=1;},
  [](const NewCohortMachine& q){assert(q.faultHit && q.pending && !q.gdnStarts && q.convStarts==119);});
 bad("last GDN proof changed",[](NewCohortMachine& q){q.badProofStage=2;q.badProofWorker=96;q.badProofWord=63;},
  [](const NewCohortMachine& q){assert(q.faultHit && q.gdnStarts==96 && q.convStarts==119);});
 m->Stage();assert(m->Wait(1));m->Verify();
 printf("PASS actual emitted QRC1/120Conv/640groups/96GDN/all6144gated+privateNEXT/119bootONCE/noReset/noHostMath/ROOTstack%u/packets%u/refusals%u;NOT original/native/physical/fullmodel/TPS\n",0x70000-m->minStack,m->packets,cases);
}
#endif
