/* SPDX-License-Identifier: MIT */
// Actual single ROOT + SAME119 QMW1 programs. Only RAW byte endpoints;
// independent old QDA observes, never supplies a numeric ROOT input.
#define PIONEER_Q38_MIXED_PROJECTION_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_projection_root_test.cpp"
#include "../PioneerQ38MixedJoinedRootLayout.h"
namespace J=PioneerQ38MixedJoinedRootLayout;
struct JoinedRootMachine:ProjectionRootMachine {
 std::array<uint32_t,64> outer={};std::map<uint64_t,uint32_t> dram;
 unsigned projections=0,acceptedJobs=0,prepareReads=0,prepared=0,attentionChecks=0,scatterWords=0;
 bool observed=false,suppressObservers=false;double maxError=0;
 JoinedRootMachine(const char* root,const char* worker,const char* qdp,const char* qda)
  :ProjectionRootMachine(root,worker,qdp,qda,131072){
  boot[0]=J::Magic;memory[J::Boot]=J::Magic;
  for(auto range:std::array<std::pair<unsigned,unsigned>,6>{{{J::Outer,256},{J::JointSummary,256},
   {J::Recipes,4*J::RecipeStride},{J::RecipeCopies,4*J::RecipeStride},{J::DenseCopies,96*256},{J::RawCopy,5120*4}}})
   for(unsigned a=range.first;a<range.first+range.second;a+=4)memory[a]=0x7fc00001;
 }
 unsigned Index(uint32_t a)override{
  if(a==0x6000)return 0;assert(Range(a,J::Code,unsigned(code.size())) && !(a&3));l1[8190]=Word(code,a-J::Code);return 8190;
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,J::Code,unsigned(code.size()));}
 uint32_t Load(uint32_t a)override{if(Range(a,J::Code,unsigned(code.size())))return Word(code,a-J::Code);return MixedRootMachine::Load(a);}
 void Store(uint32_t a,uint32_t value)override{
  if(a==0xffb20040){assert(value==1 && !pending);SubmitJoined();return;}
  if(a==J::Status+5*4 && value==5){ObservePrepared();prepared++;}
  if(a==J::Status+5*4 && value==6 && memory.at(a)!=6){ObserveAttention();attentionChecks++;}
  bool extra=Range(a,J::Header,256) || Range(a,J::Parts,119*32) || Range(a,J::Rows,119*4) || Range(a,J::Banks,32)
   || Range(a,J::MatrixHeader,64) || Range(a,J::RecipeCopies,4*J::RecipeStride) || Range(a,J::DenseCopies,96*256)
   || Range(a,J::RawCopy,5120*4) || Range(a,J::Frames,96*256) || Range(a,J::Input,7168*4) || Range(a,J::Gate,6144*4)
   || Range(a,J::JointSummary,256);
  if(extra){assert(!pending);memory[a]=value;return;}
  ProjectionRootMachine::Store(a,value);
 }
 static uint64_t Key(unsigned route,unsigned address){return uint64_t(route)<<32|address;}
 uint32_t& WordAt(unsigned route,unsigned address){return dram.at(Key(route,address));}
 void ReadDescriptor(){
  for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(J::Header+i*4);active=descriptor[7];
  for(unsigned w=0;w<119;w++)rows[w]=memory.at(J::Rows+w*4);
 }
 void ObserveProjections(){
  for(unsigned w=0;w<active;w++){
   auto& p=*child[w];assert((p.pc==p.waitPC || p.pc==p.waitPC+4) && !p.memory.at(M::Status+8) && !p.dotProbeReads && p.CorrectRows());
   for(unsigned row=0;row<p.FfnPersistentPeer::groups*16;row++)assert(WordAt(descriptor[8],descriptor[4]+descriptor[10]+(rows[w]+row)*4)==p.buffers[4096+row]);
  }
  acceptedJobs+=active;projections++;
  printf("OBSERVED ACTUAL QJR1 projection role%u all%u rows gathered/scattered from SAME119 workers;PROVISIONAL\n",descriptor[43],descriptor[11]);
 }
 void MakeObservers(){
  assert(!observed && projections==3);observed=true;unsigned n=outer[4]+1;
  for(unsigned g=0;g<4;g++){
   auto data=std::make_unique<ProducedGroup>(g,n);
   for(unsigned h=0;h<6;h++){
    DenseAttentionMachine original(qdaImage,g*6+h,n);
    for(unsigned i=0;i<3072;i++)original.raw[i]=WordAt(outer[8],outer[9]+TT_Q38_DENSE_QG+g*12288+i*4);
    for(unsigned i=0;i<256;i++){
     original.raw[(L::Key-L::Qg)/4+i]=WordAt(outer[8],outer[9]+TT_Q38_DENSE_K+g*1024+i*4);
     original.raw[(L::Value-L::Qg)/4+i]=WordAt(outer[8],outer[9]+TT_Q38_DENSE_V+g*1024+i*4);
     original.raw[(L::QWeight-L::Qg)/4+i]=WordAt(PioneerQ38ProjectionRoute(outer[12]),outer[13]+i*4);
     original.raw[(L::KWeight-L::Qg)/4+i]=WordAt(PioneerQ38ProjectionRoute(outer[14]),outer[15]+i*4);
    }
    for(unsigned i=0;i<64;i++)original.raw[(L::Rope-L::Qg)/4+i]=WordAt(outer[41],outer[40]+outer[4]*256+i*4);
    original.SetRaw();
    if(h){original.priorK=data->keys;original.priorV=data->values;}else{data->keys=original.priorK;data->values=original.priorV;}
    assert(original.Run());original.Verify();
    for(unsigned i=0;i<256;i++){
     data->query[h*256+i]=original.memory.at(A::HalfQuery+i*4);data->output[h][i]=Float(original.memory.at(A::PreGate+i*4));
    }
    for(unsigned i=0;i<128;i++){data->currentK[i]=original.memory.at(A::NewK+i*4);data->currentV[i]=original.memory.at(A::NewV+i*4);}
    data->maximum[h]=original.memory.at(A::Summary+53*4);data->inverse[h]=original.memory.at(A::Summary+55*4);
    data->probability[h].resize(n);for(unsigned t=0;t<n;t++)data->probability[h][t]=original.memory.at(A::Scores+t*4);
   }
   oracle[g]=std::move(data);for(unsigned w=0;w<24;w++)child[g*24+w]->input=oracle[g].get();
  }
 }
 void ObservePrepared(){
  assert(observed && prepareReads==15);
  for(unsigned g=0;g<4;g++){
   DensePrepareMachine check(qdpImage,g,outer[4]);
   for(unsigned i=0;i<3072;i++)check.raw[i]=WordAt(outer[8],outer[9]+TT_Q38_DENSE_QG+g*12288+i*4);
   for(unsigned i=0;i<256;i++){
    check.raw[(L::Key-L::Qg)/4+i]=WordAt(outer[8],outer[9]+TT_Q38_DENSE_K+g*1024+i*4);
    check.raw[(L::Value-L::Qg)/4+i]=WordAt(outer[8],outer[9]+TT_Q38_DENSE_V+g*1024+i*4);
    check.raw[(L::QWeight-L::Qg)/4+i]=WordAt(PioneerQ38ProjectionRoute(outer[12]),outer[13]+i*4);
    check.raw[(L::KWeight-L::Qg)/4+i]=WordAt(PioneerQ38ProjectionRoute(outer[14]),outer[15]+i*4);
   }
   for(unsigned i=0;i<64;i++)check.raw[(L::Rope-L::Qg)/4+i]=WordAt(outer[41],outer[40]+outer[4]*256+i*4);
   check.SetRaw();assert(check.Run());check.Verify();
   for(unsigned i=0;i<1536;i++)assert(memory.at(J::Input+(g*1536+i)*4)==check.memory.at(L::Query+i*4)
    && memory.at(J::Gate+(g*1536+i)*4)==check.memory.at(L::Gate+i*4));
   for(unsigned i=0;i<128;i++)assert(memory.at(J::NewK+g*512+i*4)==check.memory.at(L::PackedK+i*4)
    && memory.at(J::NewV+g*512+i*4)==check.memory.at(L::PackedV+i*4));
  }
  puts("OBSERVED ACTUAL ROOT prepare ALL4 groups BIT-EXACT with separate QDP observer;NO observer products supplied");
 }
 void ObserveAttention(){
  assert(prepared==1 && projections==3);double delta=0,norm=0;
  for(unsigned g=0;g<4;g++)for(unsigned h=0;h<6;h++){
   assert(memory.at(J::Parameters+(g*16+h)*4)==oracle[g]->maximum[h]
    && memory.at(J::Parameters+(g*16+8+h)*4)==oracle[g]->inverse[h]);
   for(unsigned i=0;i<256;i++){
    float gate=Float(memory.at(J::Gate+(g*1536+h*256+i)*4));double e=exp(-fabs(double(gate))),sigmoid=gate>=0?1/(1+e):e/(1+e);
    float wanted=oracle[g]->output[h][i]*float(sigmoid),actual=Float(memory.at(J::Output+(g*1536+h*256+i)*4));
    assert(isfinite(actual));delta+=(double(actual)-wanted)*(double(actual)-wanted);norm+=double(wanted)*wanted;
   }
  }
  for(unsigned w=0;w<96;w++){
   auto& p=*child[w];const auto& f=p.denseHeader;
   for(unsigned h=0;h<6;h++)for(unsigned t=0;t<f[49];t++)assert(p.memory.at(W::Scores+(h*384+t)*4)==oracle[f[5]]->probability[h][f[48]+t]);
  }
  double error=sqrt(delta/(norm?norm:1));assert(error<4e-6);maxError=std::max(maxError,error);
  printf("OBSERVED ACTUAL attention6144/max-inverse+EVERY finalF16 probability vs QDA observer/L2%.12g -> O input\n",error);
 }
 void SubmitJoined(){
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];bool bank=false;
  for(unsigned b=0;b<7;b++)if(route==PioneerQ38ProjectionRoute(b))bank=true;
  if(bank){
   unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
   assert(bytes && bytes<=16384 && !(bytes&63) && !(remote&63) && local==J::Scratch && !command[1] && !command[4] && !command[6] && !command[9]
    && (write?command[2]:command[5])==129 && (write || !command[7]));
   packets++;if(packets==failPacket){pending=true;return;}
   if(write){
    ReadDescriptor();assert(remote>=descriptor[4]+descriptor[10] && remote+bytes<=descriptor[4]+descriptor[10]+descriptor[12]);
    for(unsigned i=0;i<bytes/4;i++)dram[Key(route,remote+i*4)]=memory.at(local+i*4);
    scatterWords+=bytes/4;if(remote+bytes==descriptor[4]+descriptor[10]+descriptor[12])ObserveProjections();
   }else{
    if(!observed && !suppressObservers)MakeObservers();prepareReads++;
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=WordAt(route,remote+i*4);
   }
   counts[write?4:0]++;counts[write?5:1]+=bytes/64;counts[write?7:3]++;counts[2]++;counts[6]++;
   if(packets==counterFaultPacket)counts[3]++;
   if(packets==sourceFaultPacket)source[11]^=1;
   if(packets==headerFaultPacket)memory[J::Header+48*4]^=1;
   if(packets==bootFaultPacket)memory[J::Boot+8*4]^=1;
   return;
  }
  if(memory.at(J::Header)==Q::Magic){ReadDescriptor();ProjectionRootMachine::Submit();}
  else{
   if(write && command[3]==M::Doorbell){
    auto& p=*child[Worker(route)];
    if(!p.denseActive){
     // Fold completed matrix packet bookkeeping into the SAME cumulative
     // physical counters before selecting the dense observer. No hardware
     // counter, private sequence, PC/register or tensor array is reset.
     uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=p.Count(i);
     for(unsigned i=0;i<8;i++)p.countBase[i]=physical[i];p.requests=p.responses=0;
    }
   }
   MixedRootMachine::Submit();
  }
 }
 void StageJoined(unsigned tokens,unsigned changed,unsigned layer=3){
  MixedRootMachine::Stage(tokens,layer);projections=acceptedJobs=prepareReads=prepared=attentionChecks=scatterWords=0;observed=false;dram.clear();
  const unsigned first[]={J::Magic,1,64,layer,tokens-1,129,header[6]-3,layer%7,PioneerQ38ProjectionRoute(layer%7),0,5120,5120};
  for(unsigned i=0;i<12;i++)outer[i]=first[i];outer[9]=canonical->Base+canonical->Workspace(layer%7)->offset;
  for(unsigned role:{8U,9U})for(unsigned i=0;i<canonical->Count();i++){
   auto* p=canonical->Part(i);if(p->kind==canonical->WeightKind && p->layer==layer && p->role==role){
    unsigned at=12+(role-8)*2;assert(p->rows==1 && p->rowBytes==1024);outer[at]=p->bank;outer[at+1]=canonical->Base+p->offset;
   }
  }
  for(unsigned i=0;i<16;i++)outer[16+i]=source[i];for(unsigned i=0;i<8;i++)outer[32+i]=boot[8+i];
  const unsigned fields[]={outer[9]+J::RopeOffset,outer[8],8192,64,J::Recipes,J::RecipeCopies,J::DenseCopies,J::RawCopy,sequence,119,J::Frames,J::Raw,123,5120,67108864};
  for(unsigned i=0;i<15;i++)outer[40+i]=fields[i];assert(J::OuterShape(outer.data()));
  for(unsigned i=0;i<64;i++)memory[J::Outer+i*4]=outer[i];
  for(unsigned i=0;i<5120;i++)memory[J::Raw+i*4]=Bits(float(int((i*37+changed*41)%257)-128)*.03125f);
  uint32_t sources[1920];for(unsigned i=0;i<16;i++)sources[i]=source[i];for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->source[i];
  auto plan=std::make_unique<PioneerQ38DenseProjectionPlan>();assert(plan->Build(*canonical,layer,tokens-1,123,layer%7,outer[6],{1,2,3,4},sources));
  for(unsigned slot=0;slot<4;slot++){
   unsigned address=J::Recipes+slot*J::RecipeStride;for(unsigned i=0;i<J::RecipeStride/4;i++)memory[address+i*4]=0;
   for(unsigned i=0;i<64;i++)memory[address+i*4]=plan->Frame(slot)[i];for(unsigned i=0;i<16;i++)memory[address+256+i*4]=plan->Header(slot)[i];
   for(unsigned w=0;w<119;w++)for(unsigned i=0;i<8;i++)memory[address+320+w*32+i*4]=plan->Parts(slot)[w][i];
   for(unsigned w=0;w<119;w++)memory[address+4128+w*4]=plan->Rows(slot)[w];for(unsigned i=0;i<8;i++)memory[address+4604+i*4]=plan->Banks()[i];
  }
  for(unsigned i=0;i<256;i++){
   dram[Key(PioneerQ38ProjectionRoute(outer[12]),outer[13]+i*4)]=Bits(float(1+sin(i*.013)*.1));
   dram[Key(PioneerQ38ProjectionRoute(outer[14]),outer[15]+i*4)]=Bits(float(1+cos(i*.019)*.13));
  }
  for(unsigned i=0;i<32;i++){
   float angle=float(tokens-1)*powf(10000000.f,-float(i)/32);
   dram[Key(outer[41],outer[40]+(tokens-1)*256+i*8)]=Bits(cosf(angle));dram[Key(outer[41],outer[40]+(tokens-1)*256+i*8+4)]=Bits(sinf(angle));
  }
  for(unsigned i=0;i<7168;i++)memory[J::Input+i*4]=0x7fc00001;
  for(unsigned i=0;i<6144;i++)memory[J::Gate+i*4]=memory[J::Output+i*4]=0x7fc00001;
  for(unsigned w=0;w<119;w++)firstNonce[w]=child[w]->memory.at(M::Status+4);
 }
 bool WaitJoined(unsigned n,bool quiet=false){
  while(pc!=0x4009c && !(pc==0x4004c && memory.at(J::Status+4)==n))StepScalar();
  if(pc==0x4009c){assert(memory.at(J::Status+8)==J::Fault);if(!quiet)fprintf(stderr,"JOIN_FAULT phase%u projections%u packets%u sp%x\n",memory.at(J::Status+20),projections,packets,regs[2]);return false;}
  assert(memory.at(J::Status+12)==J::Magic && !memory.at(J::Status+8) && !memory.at(J::Status+16));return true;
 }
 void VerifyJoined(){
  assert(projections==4 && acceptedJobs==366 && prepared==1 && attentionChecks==1 && prepareReads==15 && memory.at(J::JointSummary)==J::Magic
   && memory.at(J::JointSummary+4)==sequence && scatterWords==19456);
  for(unsigned w=0;w<119;w++)assert(child[w]->memory.at(M::Status+4)==firstNonce[w]+(w<64?7:w<96?5:2));
 }
};
static unsigned JoinedRefusals(JoinedRootMachine& base){
 unsigned refused=0;
 auto branch=[&](unsigned kind,unsigned arg,bool norm){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   auto& m=base;
   if(kind==0)m.memory[J::Outer+arg*4]^=0x80000000U;
   if(kind==1)m.memory[J::Recipes+arg*4]^=0x80000000U;
   if(kind==2)m.memory[J::Recipes+320+arg*32+3*4]=1;
   if(kind==3)m.memory[J::Frames+arg*256+4*4]=8192;
   if(kind==4)m.memory[J::Raw+5119*4]=0x7fc00000;
   if(kind==5)m.counts[arg]++;
   if(kind==6)m.failPacket=m.packets+1;
   if(kind==7)m.counterFaultPacket=m.packets+1;
   if(kind==8)m.badReadback=m.readbacks+arg;
   if(kind==9)m.sourceFaultPacket=m.packets+1;
   if(kind==10)m.headerFaultPacket=m.packets+1;
   if(kind==11)m.bootFaultPacket=m.packets+1;
   if(kind==12)m.memory[J::Raw+5119*4]^=1;
   if(kind==13)m.memory[J::Recipes+3*J::RecipeStride+320+118*32+3*4]=1;
   if(kind==14)m.memory[J::Frames+95*256+40*4]^=1;
   if(kind==15){m.suppressObservers=true;m.WordAt(PioneerQ38ProjectionRoute(m.outer[12]),m.outer[13]+255*4)=0x7fc00000;}
   if(kind==16){m.suppressObservers=true;m.WordAt(m.outer[41],m.outer[40]+m.outer[4]*256+63*4)=0x7f800000;}
   unsigned start=m.packets;bool okay=m.WaitJoined(m.sequence,true);
   if(okay)fprintf(stderr,"JOIN_REFUSAL kind%u arg%u unexpectedly succeeded\n",kind,arg);
   assert(!okay && m.memory.at(J::JointSummary)!=J::Magic && !m.prepared && !m.attentionChecks);
   if(!norm)assert(!m.projections && !m.packets);
   else assert(m.projections==3 && m.packets>start);
   unsigned packets=m.packets;for(unsigned i=0;i<32;i++)m.StepScalar();assert(m.packets==packets);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
 };
 for(unsigned word:{0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,14U,16U,31U,32U,39U,40U,41U,42U,43U,44U,45U,46U,47U,48U,49U,50U,51U,52U,53U,54U,63U})branch(0,word,false);
 for(unsigned word:{0U,7U,40U,42U,43U,45U,63U})branch(1,word,false);
 for(unsigned worker:{0U,63U,118U})branch(2,worker,false);
 for(unsigned worker:{0U,95U})branch(3,worker,false);
 branch(4,0,false);for(unsigned i=0;i<8;i++)if(i!=6)branch(5,i,false);
 while(!base.AtSubmitRead(base.outer[13],1024)){assert(base.pc!=0x4009c);base.StepScalar();}
 assert(base.projections==3 && !base.prepared && !base.observed);
 branch(6,0,true);branch(7,0,true);for(unsigned word=1;word<=10;word++)branch(8,word,true);
 for(unsigned kind=9;kind<=16;kind++)branch(kind,0,true);
 printf("PASS ACTUAL QJR1 refusals%u/from actual waiting/pending-submit continuations/no full result or attention dispatch/fault parks inert;partial projections PROVISIONAL\n",refused);
 return refused;
}
int main(int argc,char** argv){
 assert(argc==5);(void)&ProjectionRefusals;(void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START ACTUAL QJR1 ONE ROOT/SAME119 workers/QG-K-V -> ROOT prepare -> retained attention -> actual O;NOT native timing");
 auto m=std::make_unique<JoinedRootMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitJoined(0));
 m->StageJoined(33,0);unsigned refused=JoinedRefusals(*m);
 for(unsigned trial=0;trial<2;trial++){
  if(trial)m->StageJoined(65,trial,7);assert(m->WaitJoined(m->sequence));m->VerifyJoined();
  printf("PASS ACTUAL QJR1 frame%u N%u/all6144 attention values used by O/ALL5120 final outputs/maxL2%.12g/SAME119 noSTOP or reboot/stack%u\n",m->sequence,trial?65:33,m->maxError,0x70000-m->minStack);
 }
 printf("PASS joined instruction graph/refusals%u;SYNTHETIC RAW learned/KV/table endpoints ONLY;NO native/original full64/stateKVtokencommit/throughput grant\n",refused);
}
