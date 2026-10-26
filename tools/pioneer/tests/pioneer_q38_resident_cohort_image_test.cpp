/* SPDX-License-Identifier: MIT */
// ACTUAL ROOT+119 ACTUAL persistent peers. ONLY byte endpoints/time simulated.
// No synthetic success receipts or numeric products are fed to the program.
#ifndef PIONEER_Q38_COHORT_GDN_PEER_ALREADY_INCLUDED
#define PIONEER_Q38_RESIDENT_GDN_OPERATOR_SERVICE_MACHINE_ONLY
#include "pioneer_q38_resident_gdn_operator_service_image_test.cpp"
#endif
#ifndef PIONEER_Q38_COHORT_PEER_BASE
#define PIONEER_Q38_COHORT_PEER_BASE ResidentGdnOperatorPeer
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentCohortShape.h"
struct CohortFabric;
struct CohortPeer:PIONEER_Q38_COHORT_PEER_BASE {
 CohortFabric* fabric=nullptr;bool active=false;unsigned launched=0;
 using PIONEER_Q38_COHORT_PEER_BASE::PIONEER_Q38_COHORT_PEER_BASE;
 void SubmitConvBytes();void SubmitGdnBytes();void Launch(unsigned sequence);
#ifdef PIONEER_Q38_COHORT_OUTPUT_PEER
 void SubmitActivation()override;
#endif
 void Store(uint32_t at,uint32_t value)override{
#ifdef PIONEER_Q38_COHORT_OUTPUT_PEER
  if(!gdnActive && !convActive){PIONEER_Q38_COHORT_PEER_BASE::Store(at,value);return;}
#endif
  if(at==0xffb20040){assert(value==1 && !pending);if(gdnActive)SubmitGdnBytes();else {assert(convActive);SubmitConvBytes();}return;}
  PIONEER_Q38_COHORT_PEER_BASE::Store(at,value);
 }
 uint32_t ReadCycleCsr(unsigned csr)override;
};
struct CohortFabric:Machine {
 static bool Range(uint32_t at,uint32_t first,unsigned bytes){return at>=first && at-first<bytes;}
 std::array<std::vector<uint8_t>,4> sections;
 std::vector<uint32_t> memory=std::vector<uint32_t>(0x180000/4,0xdeadbeef),manifest=std::vector<uint32_t>(TT_Q38_COHORT_WORDS);
 std::map<uint64_t,uint32_t> gddr;
 std::array<std::unique_ptr<CohortPeer>,119> peers;
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 uint32_t source[16]={},counts[8]={},command[10]={},fp[32]={};
 std::array<bool,96> cleared={},verified={};
 unsigned captures=0,packets=0,nocReads=0,rootConvPackets=0,rootGroups=0,minStack=0x70000;
 unsigned convLaunches=0,gdnLaunches=0,failPacket=0,dropClearWorker=0,badProofWorker=0,badProofWord=0,badProofOperation=7;
 unsigned peerSourceFault=0,peerCounterFault=0,badPcWorker=0,badResetWorker=0;
 unsigned pausedWorker=0;
 unsigned lateManifestPacket=0,lateSourcePacket=0;
 bool pending=false,unknown=false;uint64_t clock=UINT64_C(1)<<40;
 unsigned donePc=0,parkPc=0,faultPc=0;
 static uint64_t Key(unsigned route,unsigned at){return uint64_t(route)<<32|at;}
 uint32_t Gddr(unsigned route,unsigned at)const{return gddr.at(Key(route,at));}
 void Put(unsigned route,unsigned at,uint32_t bits){gddr[Key(route,at)]=bits;}
 uint32_t& Mem(unsigned at){assert(!(at&3) && at<0x180000);return memory.at(at/4);}
 static unsigned Offset(unsigned i){return FfnPersistentPeer::CounterOffset(i);}
 static unsigned SrcOffset(unsigned i){return BlackholeDramSourceOffset(i);}
 unsigned Symbol(const char* path,const char* name){std::ifstream in(std::string(path)+"/program.symbols");std::string line;
  while(std::getline(in,line)){unsigned at=0;char type=0,text[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,text)==3 && !strcmp(text,name))return at;}assert(false);return 0;}
 CohortFabric(const char* root,const char* worker,unsigned layer=0,unsigned bank=0,unsigned position=0,unsigned slot=0,const unsigned* panels=nullptr){
#ifndef PIONEER_Q38_COHORT_OUTPUT_PEER
  assert(!panels);
#endif
  assert(canonical->Build());const char* names[]={"boot","text","conv","cohort"};
  for(unsigned i=0;i<4;i++)sections[i]=Read((std::string(root)+"/"+names[i]+".bin").c_str());
  donePc=Symbol(root,"cohort_done");parkPc=Symbol(root,"cohort_park");faultPc=Symbol(root,"cohort_fault_park");
  l1[0]=Word(sections[0],0);for(auto& c:counts)c=0xfffffff8;
  for(unsigned i=0;i<16;i++)source[i]=i?0x12340000+i:0x16005;
  auto& m=manifest;m[0]=TT_Q38_RESIDENT_COHORT_MARKER;m[1]=1;m[2]=TT_Q38_COHORT_WORDS;m[3]=layer;m[4]=position;
  m[5]=17;m[6]=18;m[7]=bank;m[8]=canonical->Base+canonical->Workspace(bank)->offset;m[9]=PioneerQ38ProjectionRoute(bank);
  m[10]=129;m[11]=119;m[12]=96;m[13]=640;m[14]=slot;m[32]=1;m[34]=2;m[36]=3;m[38]=4;m[40]=120;m[41]=384;m[42]=192;
  for(unsigned i=0;i<16;i++)m[16+i]=m[TT_Q38_COHORT_SOURCE_WORD+i]=source[i];
  PioneerQ38GdnIdentity identity={1,2,3,4};
  for(unsigned w=0;w<120;w++){
   if(w){
#ifdef PIONEER_Q38_COHORT_OUTPUT_PEER
    auto p=std::make_unique<CohortPeer>(worker,w,panels?panels[w-1]:1);p->fabric=this;
    for(unsigned i=0;i<16;i++)p->source[i]+=w*0x100;
    p->memory[0x7404c]=p->source[0];assert(p->BootNew());
#else
    auto p=std::make_unique<CohortPeer>(worker,w);p->fabric=this;assert(p->BootNew());
#endif
    for(unsigned i=0;i<16;i++)m[TT_Q38_COHORT_SOURCE_WORD+w*16+i]=p->source[i];
    if(w<=96){
     p->StageGdn(2,18,layer,bank,position,slot,1);
     for(unsigned i=0;i<64;i++){
      m[TT_Q38_COHORT_GDN_WORD+(w-1)*192+i]=p->input[i];m[TT_Q38_COHORT_GDN_WORD+(w-1)*192+64+i]=p->state[i];
      m[TT_Q38_COHORT_GDN_WORD+(w-1)*192+128+i]=p->gated[i];
     }
     const auto* in=p->input.data();const auto* st=p->state.data();const auto* ga=p->gated.data();
     for(unsigned i=0;i<16;i++){
      Put(in[13],in[12]+i*4,Bits(-float(i+1)*.25f));Put(in[41],in[15]+i*4,Bits(float(i+1)*.01f));
      Put(in[8],in[4]+in[45]+i*4,Bits(float(i)*.13f-2));Put(in[8],in[4]+in[46]+i*4,Bits(float(i)*.07f-2));
     }
     for(unsigned i=0;i<128;i++){
      Put(ga[13],ga[12]+i*4,p->norm[i]);Put(ga[8],ga[4]+ga[42]+i*4,p->z[i]);
     }
     for(unsigned i=0;i<8192;i++){
      p->prior[i]=Bits(float(sin((i+(w-1)*8192)*.003))*0.0001f);
      Put(st[12],st[5]+i*4,p->prior[i]);Put(st[13],st[7]+i*4,0x7fc00000);
     }
     for(unsigned i=0;i<16;i++)p->memory[TT_Q38_GDN_GATED_READY+i*4]=0xfeed0000+w*16+i;
    }
    p->StageConv(1,17,layer,bank,position);p->memory[0x74100]=0;
    for(unsigned i=0;i<384;i++)m[TT_Q38_COHORT_CONV_WORD+w*384+i]=p->table[i];
    peers[w-1]=std::move(p);
   }else {
    auto plan=std::make_unique<PioneerQ38ConvBatchPlan>();assert(plan->Build(*canonical,layer,position,bank,0,identity,source,17));
    for(unsigned i=0;i<384;i++)m[TT_Q38_COHORT_CONV_WORD+i]=plan->Records()[i];
   }
   const auto* table=PioneerQ38CohortConv(m.data(),w);
   for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){
    const auto* d=table+j*64;unsigned first=d[7];
    for(unsigned c=0;c<16;c++){
     uint32_t input=Bits(float(sin((first+c)*.017+.231)));Put(d[8],d[4]+d[45]+c*4,input);
     for(unsigned t=0;t<4;t++)Put(d[13],d[12]+(c*4+t)*4,Bits(float(int(((first+c)*7+t*11)%29)-14)*.03125f));
     for(unsigned t=0;t<3;t++)Put(d[41],d[15]+(c*3+t)*4,Bits(float(cos((first+c)*.013+t*.117+.031))));
     Put(d[8],d[4]+d[43]+c*4,0x7fc00000);
    }
   }
  }
  for(unsigned w=1;w<=96;w++)peers[w-1]->peer=peers[(w-1)^1].get();
  assert(PioneerQ38ResidentCohortShape(m.data()));
  for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)Mem(TT_Q38_COHORT_MANIFEST+i*4)=m[i];
  Mem(0x17ffc0)=Mem(0x17ffc4)=Mem(0x17ffc8)=0;
 }
 virtual bool RangeData(unsigned at)const{return Range(at,0x74000,0x1800) || Range(at,0x80000,0x1100)
  || Range(at,TT_Q38_COHORT_MANIFEST,TT_Q38_COHORT_WORDS*8) || Range(at,TT_Q38_COHORT_COUNTERS,0x1200)
  || Range(at,0x6e000,8192) || Range(at,0x17ffc0,12);}
 unsigned CodeSection(unsigned at)const{return Range(at,0x72000,unsigned(sections[1].size()))?1:Range(at,0x64000,unsigned(sections[2].size()))?2:3;}
 unsigned CodeBase(unsigned i)const{return i==1?0x72000:i==2?0x64000:0xb8000;}
 bool CodeAddress(uint32_t at)const override{return at==0x6000 || Range(at,0x72000,unsigned(sections[1].size()))
  || Range(at,0x64000,unsigned(sections[2].size())) || Range(at,0xb8000,unsigned(sections[3].size()));}
 unsigned Index(uint32_t at)override{if(at==0x6000)return 0;assert(CodeAddress(at) && !(at&3));unsigned i=CodeSection(at);l1[8180]=Word(sections[i],at-CodeBase(i));return 8180;}
 unsigned StepLimit()const override{return 1000000000;}
 uint32_t Load(uint32_t at)override{
  if(CodeAddress(at))return at==0x6000?l1[0]:Word(sections[CodeSection(at)],at-CodeBase(CodeSection(at)));
  if(Range(at,0xffb20000,0x2000)){
   nocReads++;unsigned offset=at-0xffb20000;
   for(unsigned i=0;i<8;i++)if(offset==Offset(i))return counts[i];
   for(unsigned i=0;i<16;i++)if(offset==SrcOffset(i))return source[i];
   if(offset<40)return command[offset/4];
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
   assert(false && "unknown ROOT NoC register");
  }
  if((at&3) || !RangeData(at))fprintf(stderr,"UNQUALIFIED ROOT load at%08x PC%08x steps%u packets%u\n",at,pc,steps,packets);
  assert(!(at&3) && RangeData(at));return Mem(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!CodeAddress(at));
  if(Range(at,0xffb20000,40)){assert(!pending && !unknown);command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending && !unknown);Submit();return;}
  assert(!(at&3) && RangeData(at) && !Range(at,TT_Q38_COHORT_MANIFEST,TT_Q38_COHORT_WORDS*4));
  if(Range(at,TT_Q38_COHORT_SHADOW,TT_Q38_COHORT_WORDS*4)){
   assert(!packets && captures<TT_Q38_COHORT_WORDS && at==TT_Q38_COHORT_SHADOW+captures*4 && value==Mem(TT_Q38_COHORT_MANIFEST+captures*4));captures++;
  }
  if(at==0x17ffc8 && value==TT_Q38_RESIDENT_COHORT_FAULT)unknown=true;
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,at);
  Mem(at)=value;
 }
 virtual void Submit(){
  packets++;if(packets==failPacket){pending=true;return;}
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(!command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
  if(Range(pc,0x64000,unsigned(sections[2].size()))){
   unsigned j=rootConvPackets/5,p=rootConvPackets%5;const auto* d=PioneerQ38CohortConv(manifest.data(),0)+j*64;assert(j<6);
   const unsigned locations[]={0x80000,0x80400,0x80800,0x80c00,0x81000},sizes[]={64,256,192,192,64};
   const unsigned remotes[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},routes[]={d[8],d[13],d[41],d[8],d[8]};
   assert(local==locations[p] && remote==remotes[p] && route==routes[p] && bytes==sizes[p] && write==(p>=3));
   for(unsigned i=0;i<bytes/4;i++){if(write)Put(route,remote+i*4,Mem(local+i*4));else Mem(local+i*4)=Gddr(route,remote+i*4);}
   rootConvPackets++;if(p==4)rootGroups++;
  }else {
   unsigned w=1;while(w<120 && PioneerQ38ConvPeer(w)!=route)w++;assert(w<120 && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));auto& p=*peers[w-1];
   if(remote>=0xffb00000)assert(!write && bytes==4); // NO grouped/reserved MMIO reads
   if(write){
    assert(!p.active);
    for(unsigned i=0;i<bytes/4;i++)if(!(remote==TT_Q38_GDN_GATED_READY && w==dropClearWorker))p.memory[remote+i*4]=Mem(local+i*4);
    if(remote==TT_Q38_GDN_GATED_READY){assert(bytes==64);for(unsigned i=0;i<16;i++)assert(!Mem(local+i*4));cleared[w-1]=true;}
    if(remote==0x74100){assert(bytes==4);unsigned op=p.memory.at(0x74034);if(op==8){for(unsigned i=0;i<96;i++)assert(cleared[i] && verified[i]);assert(Mem(TT_Q38_COHORT_STATS+20)==120);gdnLaunches++;}else {assert(op==7);convLaunches++;}p.Launch(Mem(local));}
   }else {
    for(unsigned i=0;i<bytes/4;i++){
     uint32_t value=0;unsigned at=remote+i*4;
     if(at==0xffb13140)value=w==badPcWorker?0xdeadc0dc:p.pc;else if(at==0xffb121b0)value=w==badResetWorker?0:0x46800;
     else if(remote>=0xffb00000){bool found=false;for(unsigned k=0;k<16;k++)if(at==0xffb20000+SrcOffset(k)){value=p.source[k];found=true;}
      for(unsigned k=0;k<8;k++)if(at==0xffb20000+Offset(k)){value=p.Count(k);if(w==peerCounterFault && k==0 && p.launched)value^=1;found=true;}assert(found);
      if(w==peerSourceFault && at==0xffb20000+SrcOffset(11) && p.launched)value^=1;
     }else value=p.memory.at(at);
     if(remote==0x83f00 && w==badProofWorker && p.memory.at(0x74034)==badProofOperation && i==badProofWord)value^=1;
     Mem(local+i*4)=value;
    }
    if(remote==TT_Q38_GDN_GATED_READY){assert(bytes==64 && !gdnLaunches);bool zero=true;for(unsigned i=0;i<16;i++)zero &= !Mem(local+i*4);verified[w-1]=zero;}
   }
  }
  unsigned base=write?4:0;counts[base]++;counts[base+1]+=(bytes+63)/64;counts[base+3]++;counts[2]++;counts[6]++;
  if(packets==lateManifestPacket)Mem(TT_Q38_COHORT_MANIFEST+4*4)^=1;
  if(packets==lateSourcePacket)source[11]^=1;
 }
 static float Flush(float f){return Bits(f)&0x7f800000?f:Float(Bits(f)&0x80000000);}
 void StepRoot(){
  clock+=19;uint32_t i=l1[Index(pc)],op=i&127;
  if((i&0x707f)==0x23){assert(++steps<StepLimit());unsigned a=(i>>15)&31,b=(i>>20)&31;uint32_t at=regs[a]+Sign(((i>>25)<<5)|((i>>7)&31),12);
   assert(Range(at,0x6e000,8192));uint32_t aligned=at&~3U,shift=(at&3)*8;Store(aligned,(Mem(aligned)&~(255U<<shift))|((regs[b]&255)<<shift));pc+=4;
  }else if(op==7 || op==0x27 || op==0x53){
   assert(++steps<StepLimit());unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
   if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(i>>20,12));}
   else if(op==0x27){assert(f==2);Store(regs[s]+Sign((hi<<5)|d,12),fp[t]);}
   else {float a=Flush(Float(fp[s])),b=Flush(Float(fp[t]));switch(hi){
    case 0:assert(f==7 || !f);fp[d]=Bits(Flush(a+b));break;case 4:assert(f==7 || !f);fp[d]=Bits(Flush(a-b));break;
    case 8:assert(f==7 || !f);fp[d]=Bits(Flush(a*b));break;case 0x10:assert(f<=2);fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000));break;
    case 0x50:assert(f<=2);if(d)regs[d]=f==0?a<=b:f==1?a<b:a==b;break;case 0x68:assert(t<=1);fp[d]=Bits(t?float(regs[s]):float(int32_t(regs[s])));break;
    case 0x70:assert(!t && !f);if(d)regs[d]=fp[s];break;case 0x78:assert(!t && !f);fp[d]=regs[s];break;default:assert(false);
   }}regs[0]=0;pc+=4;
  }else Machine::Step();
  if(!(steps&255))Tick();
 }
 void Tick(){for(auto& p:peers)if(p->active && p->logicalWorker!=pausedWorker){
  for(unsigned i=0;i<64;i++){
   if(p->GdnFaulted() || (p->pc==TT_Q38_PEER_WAIT_LOAD && p->memory.at(0x74144)==p->memory.at(0x74100))){p->active=false;break;}
   clock+=19;p->StepBlock();
  }
 }}
 bool Run(){while(pc!=parkPc && !(unknown && l1[Index(pc)]==0x0000006f))StepRoot();assert(minStack>=0x6e000 && captures==TT_Q38_COHORT_WORDS);return !unknown;}
 void Inert(){unsigned count=packets,reads=nocReads;for(unsigned i=0;i<128;i++)StepRoot();assert(count==packets && reads==nocReads);}
 void Verify(){
  assert(!pending && !unknown && Mem(0x17ffc4)==6144 && Mem(0x17ffc8)==TT_Q38_RESIDENT_COHORT_MARKER && Mem(0x17ffc0)==donePc);
  assert(convLaunches==119 && gdnLaunches==96 && rootGroups==6 && rootConvPackets==30 && Mem(TT_Q38_COHORT_STATS+20)==120 && Mem(TT_Q38_COHORT_STATS+24)==96);
  VerifyHistoryAndGdn();
 }
 void VerifyHistoryAndGdn(){
  for(unsigned w=0;w<120;w++){
   const auto* table=PioneerQ38CohortConv(manifest.data(),w);
   for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++)for(unsigned c=0;c<16;c++){
    const auto* d=table+j*64;float sum=0;for(unsigned t=0;t<4;t++)sum+=Float(t==3?Gddr(d[8],d[4]+d[45]+c*4):Gddr(d[41],d[15]+(c*3+t)*4))*Float(Gddr(d[13],d[12]+(c*4+t)*4));
    double x=sum,expected=x/(1+exp(-x)),actual=Float(Gddr(d[8],d[4]+d[43]+c*4));assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected)));
    for(unsigned t=0;t<3;t++)assert(Gddr(d[8],d[4]+d[42]+(c*3+t)*4)==(t==2?Gddr(d[8],d[4]+d[45]+c*4):Gddr(d[41],d[15]+(c*3+t+1)*4)));
   }
   if(w){auto& p=*peers[w-1];assert(!p.active && !p.GdnFaulted());unsigned initial=manifest[64+w-1];
    assert(Mem(TT_Q38_COHORT_SEQUENCE+(w-1)*4)==initial+(w<=96?2U:1U));
    if(w<=96){p.VerifyGdn(initial+2);for(unsigned i=0;i<64;i++)assert(Gddr(p.gated[8],p.gated[4]+p.gated[43]+i*4)==p.gatedOutput[i]);
     for(unsigned i=0;i<8192;i++)assert(Gddr(p.state[13],p.state[7]+i*4)==p.next[i]);}
   }
  }
 }
};
void CohortPeer::SubmitConvBytes(){
 ClearTensorState();unsigned n=PioneerQ38ConvBatchCount(logicalWorker);assert(convPackets<n*5);unsigned j=convPackets/5,p=convPackets%5;convPackets++;bool write=p>=3;
 const auto* d=table.data()+j*64;const unsigned local[]={0x80000,0x80400,0x80800,0x80c00,0x81000},bytes[]={64,256,192,192,64};
 const unsigned remote[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},route[]={d[8],d[13],d[41],d[8],d[8]};
 const uint32_t expected[]={write?local[p]:remote[p],0,write?selfRoute:route[p],write?remote[p]:local[p],0,write?route[p]:selfRoute,0,write?0x212U:0,bytes[p],0};
 for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
 for(unsigned i=0;i<bytes[p]/4;i++)if(write){fabric->Put(route[p],remote[p]+i*4,memory.at(local[p]+i*4));convWrites++;}
 else {assert(memory.at(local[p]+i*4)==0x7fc50000+p*1024+i);memory[local[p]+i*4]=fabric->Gddr(route[p],remote[p]+i*4);}
 unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
}
void CohortPeer::SubmitGdnBytes(){
 unsigned p=gdnPackets;
 if(p<7){const unsigned remote[]={input[4]+input[42],input[4]+input[43],input[4]+input[44],input[4]+input[45],input[4]+input[46],input[12],input[15]};
  const unsigned route[]={input[8],input[8],input[8],input[8],input[8],input[13],input[41]},words[]={128,128,64,16,16,16,16};
  for(unsigned i=0;i<words[p];i++){uint32_t bits=fabric->Gddr(route[p],remote[p]+i*4);if(p<3)raw[(p==0?0:p==1?128:256)+i]=bits;else learned[p-3][i]=bits;}
  if(p>=3)raw[320+p-3]=learned[p-3][input[47]];
 }
 if(p==7 || p==8)for(unsigned i=0;i<4096;i++)prior[(p-7)*4096+i]=fabric->Gddr(state[12],state[5]+((p-7)*4096+i)*4);
 if(p>=11 && gdnPhase==2)for(unsigned i=0;i<128;i++)norm[i]=fabric->Gddr(gated[13],gated[12]+i*4);
 if(p>=11 && gdnPhase==3)for(unsigned i=0;i<128;i++)z[i]=fabric->Gddr(gated[8],gated[4]+gated[42]+i*4);
 ResidentGdnOperatorPeer::SubmitGdn();
 if(p==9 || p==10)for(unsigned i=0;i<4096;i++)fabric->Put(state[13],state[7]+((p-9)*4096+i)*4,next[(p-9)*4096+i]);
 if(gdnPhase==5)for(unsigned i=0;i<64;i++)fabric->Put(gated[8],gated[4]+gated[43]+i*4,gatedOutput[i]);
}
void CohortPeer::Launch(unsigned sequence){
 assert(!active && sequence==launched+1 && memory.at(0x74030)==sequence);launched=sequence;unsigned op=memory.at(0x74034);
 if(op==7){convActive=activation=true;gdnActive=false;convPackets=convWrites=0;
  for(unsigned i=0;i<384;i++)table[i]=memory.at(TT_Q38_CONV_BATCH_TABLE+i*4);
 }else {assert(op==8 && logicalWorker<=96);gdnActive=activation=true;convActive=false;
  gdnPackets=gdnPhase=gdnPolls=gdnImports=gdnOps=clockReads=0;gdnNextWrites=gdnOutputWrites=gdnGatedWrites=gdnCsrReads=0;
  for(unsigned i=0;i<64;i++){assert(input[i]==memory.at(TT_Q38_GDN_INPUT_DESCRIPTOR+i*4) && state[i]==memory.at(TT_Q38_RGDN_DESCRIPTOR+i*4) && gated[i]==memory.at(TT_Q38_GDN_GATED_DESCRIPTOR+i*4));}
  for(unsigned i=0;i<8;i++)gdnBefore[i]=Count(i);
 }active=true;
}
uint32_t CohortPeer::ReadCycleCsr(unsigned csr){assert(gdnActive && (csr==0xb00 || csr==0xb80));clockReads++;return csr==0xb00?uint32_t(fabric->clock):uint32_t(fabric->clock>>32);}
#ifndef PIONEER_Q38_RESIDENT_COHORT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!strcmp(argv[3],"--smoke") || !strcmp(argv[3],"--positives") || !strcmp(argv[3],"--stalled"))));
 bool smoke=argc==4 && !strcmp(argv[3],"--smoke"),positivesOnly=argc==4;
 setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;unsigned positive=0,faults=0;
 if(argc==4 && !strcmp(argv[3],"--stalled")){
  CohortFabric f(argv[1],argv[2]);f.pausedWorker=1;assert(!f.Run() && !f.gdnLaunches);
  assert(f.Mem(TT_Q38_COHORT_STATS+16)==119+65536 && f.peers[0]->active && !f.peers[0]->memory.at(0x74144));
  f.Inert();printf("PASS PARTIAL ACTUAL stalled worker/65536bounded status polls+119initial reads/NO invented completion/NO GDN/ROOT fault inert;ROOTsteps%u packets%u;ITERATION bound only,NOT physical watchdog/timing qualification\n",f.steps,f.packets);return 0;
 }
 for(unsigned variant=0;variant<(smoke?1U:3U);variant++){
  CohortFabric f(argv[1],argv[2],variant==0?0:variant==1?62:30,variant==0?0:variant==1?6:3,variant==0?0:variant==1?8191:5,variant&1);
  assert(f.Run());f.Verify();f.Inert();positive++;
  printf("PASS ACTUAL ROOT+119persistent peer programs/ALL640 Conv groups includingROOT0six/ALL96cooperative GDN halves/6144privateoutputs/ALL96clear+readback BEFORE ANY op8/NO reset/code reload or host intermediate;variant%u ROOTsteps%u packets%u maxStack%u. SYNTHETIC bytes/time,NOT native/all64layers/token loop or physical speed\n",variant,f.steps,f.packets,0x70000-f.minStack);
 }
 if(positivesOnly){puts("PASS PARTIAL positive-only cohort/history/GDN-next observer;fault suite intentionally NOT run by this option");return 0;}
 for(unsigned word=0;word<64;word++){
  CohortFabric f(argv[1],argv[2]);f.badProofWorker=1;f.badProofWord=word;assert(!f.Run() && !f.gdnLaunches);f.Inert();faults++;
 }
 puts("PASS ALL64 actual Conv-worker receipt corruptions refused by ACTUAL ROOT before ANY GDN launch");
 for(unsigned word:{0U,8U,17U,49U,62U,63U}){
  CohortFabric f(argv[1],argv[2]);f.badProofWorker=1;f.badProofWord=word;f.badProofOperation=8;assert(!f.Run());f.Inert();faults++;
 }
 for(unsigned w:{1U,48U,96U}){
  CohortFabric f(argv[1],argv[2]);f.dropClearWorker=w;assert(!f.Run() && !f.gdnLaunches);f.Inert();faults++;
 }
 for(unsigned w:{1U,60U,119U})for(unsigned kind=0;kind<4;kind++){
  CohortFabric f(argv[1],argv[2]);if(!kind)f.peerSourceFault=w;if(kind==1)f.peerCounterFault=w;if(kind==2)f.badPcWorker=w;if(kind==3)f.badResetWorker=w;
  assert(!f.Run() && !f.gdnLaunches);f.Inert();faults++;
 }
 for(unsigned kind=0;kind<3;kind++){
  CohortFabric f(argv[1],argv[2]);if(!kind)f.failPacket=1;if(kind==1)f.lateManifestPacket=1;if(kind==2)f.lateSourcePacket=1;
  assert(!f.Run() && !f.gdnLaunches);f.Inert();faults++;
 }
 printf("PASS complete actual ROOT/actual119peer cohort positive%u/fault%u;SYNTHETIC endpoints+clock,NO native/initialized owner/current-slot/fullmodel or physical throughput grant\n",positive,faults);
}
#endif
