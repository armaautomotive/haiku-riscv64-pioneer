/* SPDX-License-Identifier: MIT */
// ACTUAL complete recurrent ROOT+SAME119 workers. Only raw hidden/learned/
// prior-state bytes are supplied. All matrix/Conv/GDN/intermediate products
// come from emitted instructions; independent observers never feed them back.
#define PIONEER_Q38_MIXED_RECURRENT_COHORT_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_cohort_test.cpp"
#include "../PioneerQ38MixedRecurrentBlockPlan.h"
#include "../PioneerQ38MixedRecurrentBlockCounters.h"
#include "pioneer_q38_projection_input_reference.h"
namespace RB=PioneerQ38MixedRecurrentBlockRootLayout;
struct RecurrentBlockMachine:NewCohortMachine {
 struct Weight {unsigned route,base,bytes,type,k,first;};
 std::map<uint64_t,Weight> weights;
 std::unique_ptr<PioneerQ38MixedRecurrentBlockPlan> block=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
 std::array<uint32_t,128> outer={};unsigned matrixStarts=0,matrixChecks=0,observed=0,finalWords=0;
 unsigned initialNorm=0,postNorm=0,activationChecks=0,gatedChecks=0,numericCohortChecks=0;
 unsigned outerFaultPacket=0,rootBootFaultPacket=0,stopJobs=0,stopStarts=0;
 uint32_t peerCounterBaseline[119][8]={}; // observed physical registers,never private completion state
 RecurrentBlockMachine(const char* image,const char* worker,bool virgin=false):NewCohortMachine(image,worker,false,virgin){
  std::array<uint32_t,1920> sources;for(unsigned i=0;i<16;i++)sources[i]=source[i];
  for(unsigned w=0;w<119;w++){
   for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->source[i];
   for(unsigned i=0;i<8;i++)peerCounterBaseline[w][i]=child[w]->Count(i);
  }
  const auto* workspace=canonical->Workspace(0);unsigned input=canonical->Base+workspace->offset+0x600000;
  assert(block->Build(*canonical,0,0,0,0,0,1001,1,{1,2,3,4},sources.data(),0,input) && block->Matches(*canonical));
  for(unsigned i=0;i<128;i++){outer[i]=block->Outer()[i];memory[RB::Outer+i*4]=outer[i];}
  for(unsigned i=0;i<64;i++){boot[i]=block->Boot(0)[i];memory[RB::Boot+i*4]=boot[i];memory[RB::Header+i*4]=0;}
  for(auto r:std::array<std::pair<unsigned,unsigned>,22>{{{RB::Recipes,4*5120},{RB::RecipeCopies,4*5120},
   {RB::Original,20480},{RB::OriginalCopy,20480},{RB::Weight,20480},{RB::PostResidual,20480},{RB::PostNormalized,20480},{RB::Final,20480},
   {RB::Input,17408*4},{RB::Output,6144*4},{RB::InputShadow,17408*4},{RB::NativeShadow,17408*2},{RB::ScaleShadow,768},
   {RB::Native,17408*2},{RB::Scales,768},{RB::Projected,17408*4},{RB::Parts,119*32},{RB::Rows,119*4},{RB::Banks,32},
   {RB::MatrixHeader,64},{RB::RecipeShadow,256},{0x76d00,0x11e0}}})
   for(unsigned a=r.first;a<r.first+r.second;a+=4)memory[a]=0x7fc00001;
  for(unsigned a=RB::QuantizedChunk;a<RB::QuantizedChunk+576;a+=4)memory[a]=0x7fc00001;
  for(unsigned a=0x92000;a<0x92800;a+=4)memory[a]=0x7fc00001;
  for(unsigned n=0;n<8;n++){
   const auto* r=block->Recipe(n);for(unsigned i=0;i<1280;i++)Put(outer[8],outer[57]+n*5120+i*4,r[i]);
   for(unsigned w=0;w<r[7];w++){
    const auto* p=r+80+w*8;unsigned size=p[2]*(r[14]/(r[15]==8?32:256))*(r[15]==8?576:2304);
    Weight span={p[4],p[0],size,r[15],r[14],r[1032+w]};assert(weights.emplace(Key(span.route,span.base),span).second);
   }
   // Poison all matrix destinations, INCLUDING the projections needed by
   // the cohort. The base primitive-output Seed() was deliberately skipped.
   for(unsigned i=0;i<r[11];i++)Put(outer[8],outer[9]+r[10]+i*4,0x7fc00000);
  }
  for(unsigned i=0;i<RC::ConvWords;i++)Put(outer[8],outer[58]+i*4,block->Conv()[i]);
  for(unsigned i=0;i<RC::GdnWords;i++)Put(outer[8],outer[59]+i*4,block->Gdn()[i]);
  SeedRaw();
 }
 uint32_t Get(unsigned route,unsigned address)const override{
  auto raw=dram.find(Key(route,address));if(raw!=dram.end())return raw->second;
  auto found=weights.upper_bound(Key(route,address));assert(found!=weights.begin());--found;const auto& s=found->second;
  assert(s.route==route && address>=s.base && address-s.base<s.bytes && !(address&3));unsigned offset=address-s.base,tile=s.type==8?576:2304;
  unsigned t=offset/tile,at=offset%tile,result=0,blocks=s.k/(s.type==8?32:256);
  for(unsigned b=0;b<4;b++){
   unsigned byte=at+b,index=byte/16,row=s.first+(t/blocks)*16+byte%16;uint16_t half=uint16_t(0x0800+(row%4)*0x80);unsigned value;
   if(s.type==8)value=index<2?(half>>(index*8))&255:unsigned(int((t*17+row*19+(index-2)*37)%255)-127)&255;
   else value=index<4?(half>>((index%2)*8))&255:index<16?1+(t+row+index)%3:(t*17+row*19+index*37)%256;
   result|=value<<(b*8);
  }
  return result;
 }
 void SeedRaw(bool seedHidden=true){
  unsigned route=outer[8],base=outer[9];
  for(unsigned i=0;i<5120;i++){
   if(seedHidden)Put(PioneerQ38ProjectionRoute(outer[68]),outer[69]+i*4,Bits(float(sin(i*.017+.27))));
   for(unsigned n=0;n<2;n++)Put(PioneerQ38ProjectionRoute(outer[64+n*2]),outer[65+n*2]+i*4,Bits(float(1+sin(i*.017+n*.37)*.1)));
   if(seedHidden || outer[68]!=outer[7] || outer[69]!=base+RB::FinalOffset)Put(route,base+RB::FinalOffset+i*4,0x7fc00000);
  }
  for(unsigned w=0;w<120;w++)for(unsigned job=0;job<PioneerQ38ConvBatchCount(w);job++){
   const auto* d=block->Conv()+w*384+job*64;
   for(unsigned c=0;c<16;c++){
    for(unsigned t=0;t<4;t++)Put(d[13],d[12]+(c*4+t)*4,Bits(float(int(((d[7]+c)*7+t*11)%29)-14)*.03125f));
    for(unsigned t=0;t<3;t++)Put(d[41],d[15]+(c*3+t)*4,Bits(float(cos((d[7]+c)*.013+t*.117+.031))));
    Put(d[8],d[4]+d[43]+c*4,0x7fc00000);
   }
  }
  for(unsigned w=0;w<96;w++){
   const auto* in=block->Gdn()+w*192;const auto* st=in+64;const auto* ga=in+128;
   for(unsigned i=0;i<16;i++){Put(in[13],in[12]+i*4,Bits(-float(i+1)*.25f));Put(in[41],in[15]+i*4,Bits(float(i+1)*.01f));}
   for(unsigned i=0;i<128;i++)Put(ga[13],ga[12]+i*4,Bits(float(.5+sin(i*.07)*.3)));
   for(unsigned i=0;i<8192;i++){Put(st[12],st[5]+i*4,Bits(float(sin((i+w*8192)*.003))*0.0001f));Put(st[13],st[7]+i*4,0x7fc00000);}
   for(unsigned i=0;i<16;i++)child[w]->memory[TT_Q38_GDN_GATED_READY+i*4]=0xfeed0000+w*16+i;
  }
 }
 unsigned Phase()const{return memory.at(RB::Status+20);}
 void Store(uint32_t a,uint32_t value)override{
  // The standalone cohort received read-only preinstalled primary tables.
  // THIS parent genuinely loads those tables from owned GDDR after all
  // four projections close. Allow exactly that NEW pre-release phase,
  // not writes to primary metadata during Conv/GDN or later consumers.
  if(Phase()==6 && (Range(a,RC::Conv,RC::ConvWords*4) || Range(a,RC::Gdn,RC::GdnWords*4))){
   assert(!pending && !convStarts && !gdnStarts && matrixStarts==4 && matrixChecks==244 && memory.at(RB::Header)==RC::Magic);
   if(a==RC::Conv)for(const auto& p:child)assert(!p->active && p->GdnFinished(p->sequence));
   memory[a]=value;return;
  }
  bool allowed=Range(a,RB::Original,20480) || Range(a,RB::OriginalCopy,20480) || Range(a,RB::Weight,20480)
   || Range(a,RB::Recipes,4*5120) || Range(a,RB::RecipeCopies,4*5120) || Range(a,RB::PostResidual,20480)
   || Range(a,RB::PostNormalized,20480) || Range(a,RB::Final,20480) || Range(a,RB::Input,17408*4) || Range(a,RB::Output,6144*4)
   || Range(a,RB::InputShadow,17408*4) || Range(a,RB::NativeShadow,17408*2) || Range(a,RB::ScaleShadow,768)
   || Range(a,RB::Native,17408*2) || Range(a,RB::Scales,768) || Range(a,RB::Projected,17408*4)
   || Range(a,RB::Parts,119*32) || Range(a,RB::Rows,119*4) || Range(a,RB::Banks,32) || Range(a,RB::MatrixHeader,64)
   || Range(a,RB::RecipeShadow,256) || Range(a,0x76d00,0x11e0) || Range(a,RB::QuantizedChunk,576) || Range(a,0x92000,2048)
   || Range(a,RB::Header,256) || Range(a,RB::StopSummary,256);
  if(allowed){assert(!pending);memory[a]=value;return;}
  if(a==RB::Status+8 && value==RB::Fault)unknown=true;
  NewCohortMachine::Store(a,value);
 }
 void Rms(unsigned input,unsigned output,unsigned norm){
  double sum=0;for(unsigned i=0;i<5120;i++){float x=Float(memory.at(input+i*4));sum+=double(x*x);}
  float scale=1.f/sqrtf(float(sum/5120.0)+1e-6f);
  for(unsigned i=0;i<5120;i++)assert(memory.at(output+i*4)==Bits((Float(memory.at(input+i*4))*scale)*Float(Get(PioneerQ38ProjectionRoute(outer[64+norm*2]),outer[65+norm*2]+i*4))));
 }
 virtual void ObserveMatrix(){
  unsigned phase=Phase();if(phase==observed)return;observed=phase;
  const unsigned phases[]={2,3,4,5,8,10,11,13};assert(matrixStarts<8 && phase==phases[matrixStarts]);
  const auto* expected=block->Recipe(matrixStarts);assert(memory.at(RB::Header+43*4)==expected[43]);
  printf("START QRB1 actual matrix%u role%u stage%u/prior closed matrix jobs%u\n",matrixStarts+1,expected[43],phase,matrixChecks);
  if(phase==2){Rms(RB::OriginalCopy,RB::Input,0);initialNorm++;puts("PASS QRB1 actual initial RMS5120 BIT-EXACT -> alpha/beta/Z/QKV");}
  if(phase==8){
   assert(numericCohortChecks==1);for(unsigned i=0;i<6144;i++)assert(memory.at(RB::Output+i*4)==Get(outer[8],outer[9]+TT_Q38_GDN_GATED_OUTPUT+i*4));
   gatedChecks++;puts("PASS QRB1 actual6144 gated F32 -> actual ROOT Q8 quantization/output projection;NO host intermediate");
  }
  if(phase==10){
   for(unsigned i=0;i<5120;i++)assert(memory.at(RB::PostResidual+i*4)==Bits(Float(memory.at(RB::OriginalCopy+i*4))+Float(Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(13)+i*4))));
   Rms(RB::PostResidual,RB::PostNormalized,1);postNorm++;puts("PASS QRB1 original residual+post-RMS5120 BIT-EXACT -> actual Q4 gate/up");
  }
  if(phase==13){
   double delta=0,norm=0;
   for(unsigned i=0;i<17408;i++){
    float a=Float(Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(2)+i*4)),b=Float(Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(3)+i*4));
    double e=exp(-fabs(double(a))),sigmoid=a>=0?1/(1+e):e/(1+e);float wanted=(a*float(sigmoid))*b,actual=Float(memory.at(RB::Input+i*4));
    assert(isfinite(actual));delta+=(double(actual)-wanted)*(double(actual)-wanted);norm+=double(wanted)*wanted;
   }
   assert(sqrt(delta/(norm?norm:1))<4e-6);activationChecks++;puts("PASS QRB1 actual SiLU(gate)*up17408/strict independent error bound -> actual Q4 down");
  }
  unsigned k=expected[14],type=expected[15],input=memory.at(RB::Header+53*4);
  for(unsigned at=0;at<k;at+=256){
   float raw[256];PioneerQ38ProjectionInput q;for(unsigned i=0;i<256;i++)raw[i]=Float(memory.at(input+(at+i)*4));ProjectionInputReference(type,raw,q);
   for(unsigned i=0;i<128;i++)assert(memory.at(RB::Native+(at/2+i)*4)==q.operand[i]);
   unsigned n=type==8?8:1,first=at>>(type==8?5:8);for(unsigned i=0;i<n;i++)assert(memory.at(RB::Scales+(first+i)*4)==q.scale[i]);
  }
  matrixStarts++;
 }
 void VerifyCohort(){
  assert(!numericCohortChecks && convStarts==119 && gdnStarts==96 && rootConvPackets==30 && matrixStarts==4);
  for(unsigned w=0;w<120;w++)for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++)for(unsigned c=0;c<16;c++){
   const auto* d=block->Conv()+w*384+j*64;float sum=0;
   for(unsigned t=0;t<4;t++)sum+=Float(t==3?Get(d[8],d[4]+d[45]+c*4):Get(d[41],d[15]+(c*3+t)*4))*Float(Get(d[13],d[12]+(c*4+t)*4));
   double expected=double(sum)/(1+exp(-double(sum))),actual=Float(Get(d[8],d[4]+d[43]+c*4));assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected)));
   for(unsigned t=0;t<3;t++)assert(Get(d[8],d[4]+d[42]+(c*3+t)*4)==(t==2?Get(d[8],d[4]+d[45]+c*4):Get(d[41],d[15]+(c*3+t+1)*4)));
  }
  for(unsigned w=0;w<96;w++){auto& p=*child[w];assert(!p.active);p.VerifyGdn(p.sequence);
   for(unsigned i=0;i<64;i++)assert(Get(p.gated[8],p.gated[4]+p.gated[43]+i*4)==p.gatedOutput[i]);}
  numericCohortChecks++;puts("PASS QRB1 actual QKV ->640 Conv groups ->96 paired GDN/private NEXT+6144 gated;independent arithmetic");
 }
 void Submit()override{
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],remote=write?command[3]:command[0],local=write?command[0]:command[3],n=command[8];
  bool bank=false;for(unsigned b=0;b<7;b++)bank |= route==PioneerQ38ProjectionRoute(b);
  if(bank && !Range(pc,RC::ConvCode,unsigned(convRom.size()))){
   assert(n && n<=16384 && !(n&63) && !(remote&63) && local==RB::Scratch && !pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129);
   if(Phase()==7 && !numericCohortChecks)VerifyCohort();
   packets++;if(packets==failPacket){pending=true;return;}
   for(unsigned i=0;i<n/4;i++)if(write)Put(route,remote+i*4,memory.at(local+i*4));else memory[local+i*4]=Get(route,remote+i*4);
   if(write && Phase()==14)finalWords+=n/4;
   unsigned at=write?4:0;counts[at]++;counts[at+1]+=n/64;counts[at+3]++;counts[2]++;counts[6]++;Inject();return;
  }
  if(memory.at(RB::Header)==RC::Magic || Range(pc,RC::ConvCode,unsigned(convRom.size()))){NewCohortMachine::Submit();return;}
  unsigned w=0;while(w<119 && M::Peer(w+1)!=route)w++;assert(w<119);auto& p=*child[w];
  assert(local==RB::Scratch+(remote&31) && !pending && !unknown && n && n<=16384-(remote&31) && !command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129);
  packets++;if(packets==failPacket){pending=true;return;}
  if(write){
   assert(!p.active && ((remote==M::Job && n==256) || (remote==M::Doorbell && n==4)));
   if(remote==M::Job){
    if(Phase()==15){assert(n==256 && memory.at(local+8)==M::Stop);stopJobs++;}else ObserveMatrix();
   }
   for(unsigned i=0;i<n/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
   if(remote==M::Doorbell){if(Phase()==15){assert(stopJobs==119);stopStarts++;}p.Received();}peerWrites++;
  }else if(remote==0xffb121b0){assert(n==4);memory[local]=badResetWorker==w+1?0x46801:0x46800;}
  else if(remote==0xffb13140){assert(n==4);memory[local]=p.pc;}
  else if(Range(remote,0xffb20000,0x2000)){
   unsigned off=remote-0xffb20000;bool found=false;
   for(unsigned i=0;i<16;i++)if(off==BlackholeDramSourceOffset(i)){memory[local]=p.source[i];found=true;}
   for(unsigned i=0;i<8;i++)if(off==FfnPersistentPeer::CounterOffset(i)){memory[local]=p.Count(i);found=true;break;}
   for(unsigned i=0;i<4;i++)if(off==0x40+i*0x800){p.ClearTensorState();memory[local]=i?0:unsigned(p.pending);found=true;}
   if(off>=0x240 && off<0x2c0){p.ClearTensorState();assert(!p.pending);memory[local]=0;found=true;}
   assert(found);
  }else if(remote==0x83f00){
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   if(n>256){assert(!p.active && p.memory.at(M::Job+8)==1 && p.CorrectRows() && !p.dotProbeReads && p.requests==p.packets.size());matrixChecks++;
    for(unsigned i=64;i<n/4;i++)memory[local+i*4]=p.buffers[4096+i-64];}
  }else for(unsigned i=0;i<n/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=(n+63)>>6;counts[at+3]++;counts[2]++;counts[6]++;Inject();
 }
 void Inject(){
  if(packets==counterFaultPacket)counts[3]++;
  if(packets==sourceFaultPacket)source[11]^=1;
  if(packets==commandFaultPacket)command[5]^=1;
  if(packets==outerFaultPacket)memory[RB::Outer+32*4]^=1;
  if(packets==rootBootFaultPacket)memory[RB::Boot+20*4]^=1;
 }
 bool WaitBlock(unsigned n){
  while(pc!=0x4009c && !(pc==0x4004c && memory.at(RB::Status+4)==n))Step();
  if(pc==0x4009c){fprintf(stderr,"QRB1_FAULT phase%u matrices%u Conv%u GDN%u packets%u SP%x\n",Phase(),matrixStarts,convStarts,gdnStarts,packets,regs[2]);return false;}
  return true;
 }
 void BeforeSubmit(unsigned phase,unsigned remote,bool write){
  // Observe a genuinely reached submit instruction; NEVER alter the PC,
  // private registers, job number or completed products to reach a phase.
  for(;;){
   assert(pc!=0x4009c);uint32_t instruction=Load(pc);
   if((instruction&0x707f)==0x2023){
    unsigned rs1=(instruction>>15)&31;int32_t imm=Sign(((instruction>>25)<<5)|((instruction>>7)&31),12);
    if(regs[rs1]+imm==0xffb20040 && Phase()==phase && command[7]==(write?0x212U:0U)
     && command[write?3:0]==remote)return;
   }
   Step();
  }
 }
 void VerifyBlock(){
  assert(!unknown && !pending && matrixStarts==8 && matrixChecks==720 && finalWords==5120 && initialNorm==1 && postNorm==1
   && activationChecks==1 && gatedChecks==1 && numericCohortChecks==1 && memory.at(RB::JointSummary)==RB::Magic && minStack>=0x6e000);
  for(unsigned i=0;i<5120;i++)assert(Get(outer[8],outer[9]+RB::FinalOffset+i*4)==Bits(Float(Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(4)+i*4))+Float(memory.at(RB::PostResidual+i*4)))
   && memory.at(RB::Original+i*4)==memory.at(RB::OriginalCopy+i*4));
  unsigned counterRefusals=0;
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];assert(!p.active && p.sequence==initial[w]+PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w));
   uint32_t actual[8],beforeDown[8];for(unsigned i=0;i<8;i++){
    actual[i]=p.Count(i);beforeDown[i]=p.memory.at(0x83f00+i*4);
    assert(p.memory.at(0x83f00+(8+i)*4)==actual[i]);
   }
   unsigned polls=UINT32_MAX,priorPolls=UINT32_MAX;
   assert(PioneerQ38RecurrentBlockCounterProof(*block,*canonical,w+1,peerCounterBaseline[w],actual,false,polls)
    && PioneerQ38RecurrentBlockCounterProof(*block,*canonical,w+1,peerCounterBaseline[w],beforeDown,true,priorPolls)
    && polls==priorPolls && polls==(w<96?p.gdnPolls:0));
   // Correlate the production DATA helper with ALL actual emitted matrix,
   // Conv and GDN packets and the real final-job receipt. The helper is
   // never a substitute for current source/status/STOP/lifetime checks.
   for(unsigned i=0;i<8;i++){
    actual[i]++;unsigned unchanged=0xa55aa55a;
    assert(!PioneerQ38RecurrentBlockCounterProof(*block,*canonical,w+1,peerCounterBaseline[w],actual,false,unchanged)
     && unchanged==0xa55aa55a);actual[i]--;counterRefusals++;
   }
  }
  printf("PASS QRB1 close-counter helper correlated ALL119 emitted instruction streams/before+after down/actual bounded GDN polls/%u channel refusals;NOT native STOP/cleanup authority\n",counterRefusals);
 }
};
static unsigned Refusals(RecurrentBlockMachine& base){
 unsigned cases=0;
 auto bad=[&](const char* name,auto change,bool noIO){
  pid_t pid=fork();assert(pid>=0);
  if(!pid){auto& m=base;change(m);m.memory[RB::Doorbell]=1;assert(!m.WaitBlock(1) && m.unknown && m.memory.at(RB::Status+8)==RB::Fault
    && m.memory.at(RB::JointSummary)!=RB::Magic && !m.matrixStarts && !m.peerWrites && !m.convStarts && !m.gdnStarts);
   if(noIO)assert(!m.packets);
   unsigned packets=m.packets;auto physical=m.counts;auto command=m.command;m.memory[RB::Doorbell]=2;
   for(unsigned i=0;i<2048;i++)m.StepScalar();assert(m.pc==0x4009c && m.packets==packets && m.counts==physical && m.command==command);
   printf("PASS QRB1 refusal %s/inert ROOT/no peer launch,reset or commit\n",name);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
 };
 for(unsigned word:{0U,2U,14U,32U,48U,50U,57U,60U,72U})bad("sealed/invalid outer",[=](RecurrentBlockMachine& m){m.memory[RB::Outer+word*4]^=1;},true);
 for(auto field:{std::make_pair(3U,3U),std::make_pair(4U,8192U),std::make_pair(6U,0U),std::make_pair(12U,2U),
  std::make_pair(65U,base.outer[65]+4),std::make_pair(69U,base.outer[69]+4)})bad("invalid geometry",[=](RecurrentBlockMachine& m){m.memory[RB::Outer+field.first*4]=field.second;},true);
 bad("ROOT boot after private capture",[](RecurrentBlockMachine& m){m.memory[RB::Boot+20*4]^=1;},true);
 bad("physical source before first IO",[](RecurrentBlockMachine& m){m.source[11]^=1;},true);
 bad("unknown first owned read",[](RecurrentBlockMachine& m){m.failPacket=1;},false);
 bad("counter discontinuity",[](RecurrentBlockMachine& m){m.counterFaultPacket=1;},false);
 bad("source discontinuity",[](RecurrentBlockMachine& m){m.sourceFaultPacket=1;},false);
 bad("command readback discontinuity",[](RecurrentBlockMachine& m){m.commandFaultPacket=1;},false);
 bad("outer changed after owned read",[](RecurrentBlockMachine& m){m.outerFaultPacket=1;},false);
 bad("boot changed after owned read",[](RecurrentBlockMachine& m){m.rootBootFaultPacket=1;},false);
 bad("bad learned recipe before first peer write",[](RecurrentBlockMachine& m){m.Put(m.outer[8],m.outer[57],0);},false);
 return cases;
}
static unsigned LateRefusals(RecurrentBlockMachine& m){
 unsigned cases=0;
 auto bad=[&](const char* name,auto change){
  pid_t pid=fork();assert(pid>=0);
  if(!pid){change(m);assert(!m.WaitBlock(1) && m.unknown && m.pc==0x4009c && m.memory.at(RB::Status+8)==RB::Fault
    && m.memory.at(RB::Status+4)!=1 && m.memory.at(RB::JointSummary)!=RB::Magic);
   unsigned packets=m.packets,writes=m.peerWrites,final=m.finalWords;auto physical=m.counts;auto command=m.command;
   m.memory[RB::Doorbell]=2;for(unsigned i=0;i<2048;i++)m.StepScalar();
   assert(m.pc==0x4009c && m.packets==packets && m.peerWrites==writes && m.finalWords==final && m.counts==physical && m.command==command);
   printf("PASS QRB1 late refusal %s/inert ROOT/no later packet,reset or commit\n",name);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
 };
 m.BeforeSubmit(6,0xffb121b0,false);assert(m.matrixChecks==244 && !m.convStarts && !m.gdnStarts);
 bad("late captured Conv primary mutation",[](RecurrentBlockMachine& q){q.memory[RC::Conv+RC::ConvWords*4-4]^=1;});
 bad("late private outer mutation before cohort admission",[](RecurrentBlockMachine& q){q.memory[RB::Outer+32*4]^=1;});
 m.BeforeSubmit(7,m.outer[9]+TT_Q38_GDN_GATED_OUTPUT,false);assert(m.convStarts==119 && m.gdnStarts==96 && m.matrixChecks==244);
 bad("unknown first gated-output read",[](RecurrentBlockMachine& q){q.failPacket=q.packets+1;});
 bad("source changed while consuming gated F32",[](RecurrentBlockMachine& q){q.sourceFaultPacket=q.packets+1;});
 m.BeforeSubmit(9,m.outer[67],false);assert(m.matrixChecks==363 && m.matrixStarts==5);
 bad("post-RMS learned read counter discontinuity",[](RecurrentBlockMachine& q){q.counterFaultPacket=q.packets+1;});
 bad("post-RMS learned read command discontinuity",[](RecurrentBlockMachine& q){q.commandFaultPacket=q.packets+1;});
 m.BeforeSubmit(12,m.outer[9]+PioneerQ38MixedRecurrentOffset(2),false);assert(m.matrixChecks==601 && m.matrixStarts==7);
 bad("unknown actual gate/up activation read",[](RecurrentBlockMachine& q){q.failPacket=q.packets+1;});
 bad("changed original hidden before activation closes",[](RecurrentBlockMachine& q){q.memory[RB::Original]^=1;});
 m.BeforeSubmit(14,m.outer[9]+RB::FinalOffset+16384,true);assert(m.matrixChecks==720 && m.finalWords==4096);
 bad("unknown second final write never publishes completion",[](RecurrentBlockMachine& q){q.failPacket=q.packets+1;});
 bad("late ROOT boot change never publishes completion",[](RecurrentBlockMachine& q){q.rootBootFaultPacket=q.packets+1;});
 return cases;
}
#ifndef PIONEER_Q38_MIXED_RECURRENT_BLOCK_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!strcmp(argv[3],"--faults") || !strcmp(argv[3],"--full") || !strcmp(argv[3],"--late"))));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 puts("START QRB1 actual whole recurrent chain/SAME119/NO supplied intermediate products;synthetic raw learned/hidden/prior-state bytes,NOT hardware");
 auto m=std::make_unique<RecurrentBlockMachine>(argv[1],argv[2]);assert(m->WaitBlock(0));
 unsigned faults=argc==4 && strcmp(argv[3],"--late")?Refusals(*m):0;
 if(argc==4 && !strcmp(argv[3],"--faults")){printf("PASS QRB1 initial fault cases%u;positive whole chain deliberately NOT run\n",faults);return 0;}
 m->memory[RB::Doorbell]=1;
 if(argc==4 && !strcmp(argv[3],"--late"))printf("PASS QRB1 exact reached-phase late faults%u;NOT native recovery/ownership\n",LateRefusals(*m));
 assert(m->WaitBlock(1));m->VerifyBlock();
 printf("PASS QRB1 whole recurrent8matrices/720 actual matrix jobs+120Conv+96GDN/direct6144/initial+postRMS/residual+FFN+final5120/ROOTstack%u/packets%u;NOT ORIGINAL/native/fullmodel/TPS\n",0x70000-m->minStack,m->packets);
}
#endif
