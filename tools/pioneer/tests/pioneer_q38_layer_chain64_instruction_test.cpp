/* SPDX-License-Identifier: MIT */
// FULL64 scheduler, SAME119 real worker histories and ONE shared byte map.
// Only raw learned/initial-state bytes are supplied. Never transplant private
// PC/register/nonce/tensor state, seed a product, or forge a native owner.
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_block_root_test.cpp"
#include "../PioneerQ38LayerChainPackage.h"
#include "PioneerQ38InstructionWindows.h"
#include "PioneerQ38InstructionProfile.h"
#include "PioneerQ38InstructionFault.h"
namespace CL=PioneerQ38LayerChainLayout;
namespace DL=PioneerQ38MixedLiveBlockRootLayout;
struct ChainPeer:NewCohortPeer {
 using NewCohortPeer::NewCohortPeer;
 void ReceiveMatrix(){
  assert(!active && (pc==waitPC || pc==waitPC+4));ClearTensorState();uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  assert(memory.at(M::Job+4)==sequence+1);sequence=memory.at(M::Job+4);denseActive=gdnActive=convActive=outputActive=activation=false;
  type=memory.at(M::Job+17*4);inputs=memory.at(M::Job+21*4);FfnPersistentPeer::groups=memory.at(M::Job+23*4);
  base=memory.at(M::Job+22*4);unsigned route=memory.at(M::Job+24*4);count=type==8?32:256;
  assert((type==8 || type==12 || type==14) && FfnPersistentPeer::groups && FfnPersistentPeer::groups<=16);
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
   for(unsigned i=0;i<tile.size();i+=4){unsigned value=bytes->Get(route,base+t*unsigned(tile.size())+i);for(unsigned b=0;b<4;b++)tiles[t][i+b]=uint8_t(value>>(b*8));}
   packets.push_back({base+t*unsigned(tile.size()),route,0x90000,unsigned(tile.size()),int(t)});
  }
  Poison();PoisonTail();for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;active=true;
 }
 void Receive(){
  unsigned op=memory.at(M::Job+8);
  if(op==1){ReceiveMatrix();return;}
  if(op<9){Received();return;}
  assert(op<=11 && !active && (pc==waitPC || pc==waitPC+4));ClearTensorState();
  uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  assert(memory.at(M::Job+4)==sequence+1);sequence=memory.at(M::Job+4);
  // Endpoint bookkeeping only. Preserve cumulative physical counters while
  // switching the tensor observer; do not touch processor or tensor arrays.
  for(unsigned i=0;i<8;i++)countBase[i]=physical[i];requests=responses=0;
  phase=op-8;phasePackets=0;denseActive=activation=true;gdnActive=convActive=outputActive=false;
  for(unsigned i=0;i<64;i++)denseHeader[i]=memory.at(MWD::Header+i*4);
  active=true;
 }
 void SubmitDense()override{
  assert(bytes && (phase==1 || phase==3));const auto& f=denseHeader;
  unsigned global=f[48]+phasePackets*32,count=std::min(32U,std::min(f[49]-phasePackets*32,f[4]-global));
  unsigned n=count*512,which=phase==3;
  const uint32_t expected[]={f[8+which]+global*512,0,f[12+which],MWD::Chunk,0,selfRoute,0,0,n,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  for(unsigned i=0;i<n/4;i++)memory[MWD::Chunk+i*4]=bytes->Get(f[12+which],f[8+which]+global*512+i*4);
  densePackets++;phasePackets++;countBase[0]++;countBase[1]+=n/64;countBase[2]++;countBase[3]++;countBase[6]++;
 }
};
struct ChainInstruction64Machine:ScalarMachine,CohortBytes {
 using RPlan=PioneerQ38MixedRecurrentBlockPlan;
 struct Span {unsigned route,base,bytes,type,k,first;};
 struct RetainedState {
  unsigned route,history;std::vector<uint32_t> convolution,state;
  std::array<std::array<unsigned,2>,96> next={};
 };
 std::vector<RetainedState> retained;
 std::array<std::unique_ptr<ChainPeer>,119> child;
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::array<std::unique_ptr<RPlan>,64> recurrent;
 std::array<std::unique_ptr<PioneerQ38MixedLiveBlockPlan>,64> dense;
 std::array<std::unique_ptr<PioneerQ38LayerChainPackage>,64> packages;
 std::map<uint64_t,uint32_t> dram;std::map<uint64_t,Span> spans;
 std::map<uint64_t,unsigned> rawZero; // immutable raw initial bytes, NEVER a product/readiness cache
 std::vector<uint8_t> bootRom,text,conv,terminal;
 std::array<uint32_t,64> boot={};std::array<uint32_t,128> outer={};
 std::array<uint32_t,16> source={};std::array<uint32_t,10> command={};std::array<uint32_t,8> counts={};
 std::array<uint32_t,5120> input={};std::array<unsigned,119> nonceBefore={};
 uint64_t cycles=UINT64_C(1)<<40;unsigned packets=0,peerWrites=0,layer=0,completed=0,releases=0;
 unsigned matrixJobs=0,convJobs=0,gdnJobs=0,denseJobs=0,stopJobs=0,matrices=0,lastMatrix=0,finalWords=0,inputReads=0;
 unsigned rmsChecks=0,quantChecks=0,activationChecks=0,gdnChecks=0,attentionChecks=0;
 unsigned minSp=0x70000;
 PioneerQ38InstructionWindows observation; // host-only instruction accounting, not card state
 PioneerQ38InstructionProfile profile{getenv("PIONEER_TEST_ROOT_PROFILE_MAP")};
 bool pending=false;
 static uint64_t Key(unsigned route,unsigned address){return uint64_t(route)<<32|address;}
 uint32_t Get(unsigned route,unsigned address)const override{
  auto raw=dram.find(Key(route,address));if(raw!=dram.end())return raw->second;
  auto zero=rawZero.upper_bound(Key(route,address));
  if(zero!=rawZero.begin()){
   --zero;unsigned bank=unsigned(zero->first>>32),first=unsigned(zero->first);
   if(bank==route && address>=first && address-first<zero->second){assert(!(address&3));return 0;}
  }
  auto it=spans.upper_bound(Key(route,address));assert(it!=spans.begin());const auto& s=(--it)->second;
  if(s.route!=route || address<s.base || address-s.base>=s.bytes)fprintf(stderr,"UNMAPPED RAW route%x address%x layer%u phase%u\n",route,address,layer,Phase());
  assert(s.route==route && address>=s.base && address-s.base<s.bytes && !(address&3));
  unsigned offset=address-s.base,tile=PioneerQ38PanelTileBytes(s.type),t=offset/tile,at=offset%tile,result=0,blocks=s.k/(s.type==8?32:256);
  for(unsigned b=0;b<4;b++){
   unsigned byte=at+b,index=byte/16,row=s.first+(t/blocks)*16+byte%16;uint16_t half=uint16_t(0x0800+(row%4)*0x80);unsigned value;
   if(s.type==8)value=index<2?(half>>(index*8))&255:unsigned(int((t*17+row*19+(index-2)*37)%255)-127)&255;
   else if(s.type==12)value=index<4?(half>>((index%2)*8))&255:index<16?1+(t+row+index)%3:(t*17+row*19+index*37)%256;
   else value=index<128?(t*17+row*19+index*37)%256:index<192?(row&1?0x55:0xaa):index<208?uint8_t(row&2?-int(1+(t+index)%3):int(1+(t+index)%3)):index<210?(half>>((index-208)*8))&255:0;
   result|=value<<(b*8);
  }
  return result;
 }
 void Put(unsigned route,unsigned address,uint32_t value)override{dram[Key(route,address)]=value;}
 void ZeroRaw(unsigned route,unsigned address,unsigned bytes){
  assert(!(address&3) && bytes && !(bytes&3) && address>=0x04000000 && uint64_t(address)+bytes<=UINT64_C(0xc4000000));
  auto next=rawZero.lower_bound(Key(route,address));
  if(next!=rawZero.end() && unsigned(next->first>>32)==route)assert(uint64_t(address)+bytes<=unsigned(next->first));
  if(next!=rawZero.begin()){
   const auto& previous=*std::prev(next);
   if(unsigned(previous.first>>32)==route)assert(uint64_t(unsigned(previous.first))+previous.second<=address);
  }
  assert(rawZero.emplace(Key(route,address),bytes).second);
 }
 void VerifyRawZeros(){
  assert(!steps && !packets && rawZero.size()==48*(640+96));uint64_t words=0;
  for(const auto& zero:rawZero){
   unsigned route=unsigned(zero.first>>32),first=unsigned(zero.first);words+=zero.second/4;
   assert(!dram.count(zero.first) && Get(route,first)==0 && Get(route,first+zero.second-4)==0);
  }
  assert(words==UINT64_C(39223296));
  // Test RAW-byte precedence BEFORE any ROOT instruction; restore the
  // original raw zero endpoint, never retain a product or completion hint.
  const auto& zero=*rawZero.begin();unsigned route=unsigned(zero.first>>32),first=unsigned(zero.first);
  Put(route,first,0x3e800000);assert(Get(route,first)==0x3e800000);
  assert(dram.erase(zero.first)==1 && Get(route,first)==0);
 }
 uint64_t Clock()const override{return cycles;}
 uint32_t RootWord(unsigned address)const override{return memory.at(address);}
 unsigned Phase()const{return memory.at(RB::Status+20);}
 void AddRecipe(const uint32_t* r){
  assert(r[15]==8 || r[15]==12 || r[15]==14);
  for(unsigned w=0;w<r[7];w++){
   const auto* p=r+80+w*8;unsigned n=p[2]*(r[14]/(r[15]==8?32:256))*PioneerQ38PanelTileBytes(r[15]);
   assert(spans.emplace(Key(p[4],p[0]),Span{p[4],p[0],n,r[15],r[14],r[1032+w]}).second);
  }
 }
 void SeedRecurrent(unsigned selected){
  const auto& p=*recurrent[selected];const auto* o=p.Outer();
  for(unsigned slot=0;slot<8;slot++)AddRecipe(p.Recipe(slot));
  for(unsigned w=0;w<120;w++)for(unsigned job=0;job<PioneerQ38ConvBatchCount(w);job++){
   const auto* d=p.Conv()+w*384+job*64;
   for(unsigned c=0;c<16;c++){
    for(unsigned t=0;t<4;t++)Put(d[13],d[12]+(c*4+t)*4,Bits(float(int(((d[7]+c)*7+t*11)%29)-14)*.03125f));
   }
   ZeroRaw(d[41],d[15],192);
  }
  for(unsigned w=0;w<96;w++){
   const auto* in=p.Gdn()+w*192;const auto* st=in+64;const auto* ga=in+128;
   for(unsigned i=0;i<16;i++){Put(in[13],in[12]+i*4,Bits(-float(i+1)*.25f));Put(in[41],in[15]+i*4,Bits(float(i+1)*.01f));}
   for(unsigned i=0;i<128;i++)Put(ga[13],ga[12]+i*4,Bits(float(.5+sin(i*.07)*.3)));
   ZeroRaw(st[12],st[5],8192*4);
  }
  for(unsigned n=0;n<2;n++)for(unsigned i=0;i<5120;i++)Put(PioneerQ38ProjectionRoute(o[64+n*2]),o[65+n*2]+i*4,Bits(float(1+sin(i*.017+n*.37)*.1)));
 }
 ChainInstruction64Machine(const char* image,const char* worker):ScalarMachine(0,0){
  assert(canonical->Build());bootRom=Read((std::string(image)+"/boot.bin").c_str());text=Read((std::string(image)+"/text.bin").c_str());
  conv=Read((std::string(image)+"/conv.bin").c_str());terminal=Read((std::string(image)+"/terminal.bin").c_str());
  assert(bootRom.size()==4 && text.size()<=131072 && conv.size()<=32768 && terminal.size()==8);l1[0]=Word(bootRom,0);
  // Bounded TEST endpoint map, not a native mapping grant. The below bytes
  // are poison before the one ROOT release; all products come from code.
  for(unsigned a=0x6e000;a<0x70000;a+=4)memory[a]=0x7fc00001;
  for(unsigned a=0x74000;a<0x180000;a+=4)memory[a]=0x7fc00001;
  std::array<uint32_t,1920> sources={};counts.fill(0xfffffff8);
  for(unsigned i=0;i<16;i++)sources[i]=source[i]=i?0x76540000+i:0x16005;
  for(unsigned w=0;w<119;w++){
   child[w]=std::make_unique<ChainPeer>(worker,w+1,8,5120);auto& p=*child[w];p.bytes=this;
   for(unsigned i=0;i<16;i++)p.boot[16+i]=sources[(w+1)*16+i]=p.source[i]+=w*0x100;
   p.boot[5]=p.source[0];for(unsigned i=0;i<64;i++)p.memory[M::Boot+i*4]=p.boot[i];
   assert(!p.steps && p.BootMixed() && !p.sequence);
  }
  for(unsigned w=0;w<96;w++)child[w]->peer=child[w^1].get();
  unsigned bank=0,workspace=canonical->Base+canonical->Workspace(bank)->offset,initial=workspace+0x600000;
  for(unsigned selected=0;selected<64;selected++){
   packages[selected]=std::make_unique<PioneerQ38LayerChainPackage>();
   assert(packages[selected]->Build(*canonical,selected,bank,{1,2,3,4},sources.data(),bank,initial));
   if(!CL::Dense(selected)){
    recurrent[selected]=std::make_unique<RPlan>();
    assert(recurrent[selected]->Build(*canonical,selected,0,0,bank,0,1001+selected*16,selected+1,{1,2,3,4},sources.data(),bank,selected?workspace+RB::FinalOffset:initial));
    SeedRecurrent(selected);
   }else{
    dense[selected]=std::make_unique<PioneerQ38MixedLiveBlockPlan>();
    assert(dense[selected]->Build(*canonical,selected,0,0,bank,1001+selected*16,selected+1,{1,2,3,4},sources.data(),bank,workspace+RB::FinalOffset));
    for(unsigned slot=0;slot<7;slot++)AddRecipe(dense[selected]->Recipe(slot));
    const auto* o=dense[selected]->Outer();
    for(unsigned n=0;n<2;n++)for(unsigned i=0;i<5120;i++)Put(PioneerQ38ProjectionRoute(o[64+n*2]),o[65+n*2]+i*4,Bits(float(1+sin(i*.017+n*.37)*.1)));
    for(unsigned n=0;n<2;n++)for(unsigned i=0;i<256;i++)Put(PioneerQ38ProjectionRoute(o[12+n*2]),o[13+n*2]+i*4,Bits(float(1+sin(i*.013+n*.31)*.1)));
    for(unsigned i=0;i<32;i++){Put(o[41],o[40]+i*8,Bits(1.f));Put(o[41],o[40]+i*8+4,0);}
   }
   unsigned address=0;assert(CL::Address(workspace,selected,address));std::array<uint8_t,16384> raw={};
   for(unsigned at=0;at<packages[selected]->Bytes();at+=raw.size()){
    unsigned bytes=std::min(unsigned(raw.size()),packages[selected]->Bytes()-at);assert(packages[selected]->Read(at,raw.data(),bytes));
    for(unsigned i=0;i<bytes;i+=4){uint32_t word=0;for(unsigned b=0;b<4;b++)word|=uint32_t(raw[i+b])<<(b*8);Put(PioneerQ38ProjectionRoute(bank),address+at+i,word);}
   }
  }
  for(unsigned i=0;i<64;i++)memory[RB::Boot+i*4]=boot[i]=packages[0]->Boot()[i];
  for(unsigned i=0;i<128;i++)memory[RB::Outer+i*4]=outer[i]=packages[0]->Outer()[i];
  for(unsigned i=0;i<16;i++)memory[RB::Status+i*4]=0;
  for(unsigned i=0;i<5120;i++)Put(PioneerQ38ProjectionRoute(bank),initial+i*4,input[i]=Bits(float(sin(i*.017+.27))));
  memory[RB::Doorbell]=1;pc=0x6000;
 }
 bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,0x40000,unsigned(text.size())) || Range(a,0x64000,unsigned(conv.size())) || Range(a,0x60000,8);}
 unsigned Index(uint32_t a)override{
  assert(CodeAddress(a) && !(a&3));if(a==0x6000)return 0;
  // Measure actual reserved SP as well as lowest written stack word. Do not
  // infer combined runtime depth from a per-function compiler frame limit.
  if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minSp=std::min(minSp,regs[2]);}
  l1[8180]=Range(a,0x40000,unsigned(text.size()))?Word(text,a-0x40000):Range(a,0x64000,unsigned(conv.size()))?Word(conv,a-0x64000):Word(terminal,a-0x60000);return 8180;
 }
 unsigned StepLimit()const override{return PioneerQ38InstructionWindows::Limit;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  if(Range(a,0xffb20000,0x2000)){
   unsigned off=a-0xffb20000;if(off<40)return command[off/4];
   for(unsigned i=0;i<4;i++)if(off==0x40+i*0x800)return i?0:unsigned(pending);
   if(Range(off,0x240,128))return 0;
   for(unsigned i=0;i<16;i++)if(off==BlackholeDramSourceOffset(i))return source[i];
   for(unsigned i=0;i<8;i++)if(off==FfnPersistentPeer::CounterOffset(i))return counts[i];assert(false);
  }
  if(!memory.count(a))fprintf(stderr,"UNMAPPED ROOT LOAD%x PC%x layer%u phase%u\n",a,pc,layer,Phase());assert(memory.count(a));return memory.at(a);
 }
 void Advance(unsigned w,unsigned limit=10000){
  auto& p=*child[w];if(!p.active)return;
  for(unsigned i=0;i<limit;i++){
   if(p.GdnFaulted() || p.GdnFinished(p.sequence) || p.pc==p.stopPC){p.active=false;break;}
   cycles+=19;p.StepBlock();
  }
  assert(!p.GdnFaulted());
 }
 void Rms(unsigned in,unsigned out,unsigned n){
  double sum=0;for(unsigned i=0;i<5120;i++){float x=Float(memory.at(in+i*4));sum+=double(x*x);}
  float scale=1.f/sqrtf(float(sum/5120.0)+1e-6f);
  for(unsigned i=0;i<5120;i++)assert(memory.at(out+i*4)==Bits((Float(memory.at(in+i*4))*scale)*Float(Get(PioneerQ38ProjectionRoute(outer[64+n*2]),outer[65+n*2]+i*4))));
  rmsChecks++;
 }
 void MatrixObserver(){
  unsigned phase=Phase();if(lastMatrix==phase)return;lastMatrix=phase;
  assert(matrices<(!CL::Dense(layer)?8U:7U));const auto* r=!CL::Dense(layer)?recurrent[layer]->Recipe(matrices):dense[layer]->Recipe(matrices);
  uint32_t descriptor[64];for(unsigned i=0;i<64;i++)descriptor[i]=r[i];
  descriptor[0]=!CL::Dense(layer)?unsigned(PioneerQ38MixedRecurrentProjectionRootLayout::Magic):unsigned(PioneerQ38MixedProjectionRootLayout::Magic);
  if(CL::Dense(layer) && matrices<4)descriptor[6]=outer[6]+(matrices==3?4:matrices);
  descriptor[13]=RB::Rows;descriptor[48]=layer+1;descriptor[49]=119;descriptor[50]=RB::Parts;descriptor[51]=RB::MatrixHeader;descriptor[52]=RB::Banks;
  descriptor[53]=!CL::Dense(layer)?(r[43]==13?unsigned(RB::Output):unsigned(RB::Input)):
   matrices<4?(matrices==3?unsigned(DL::Output):unsigned(DL::Input)):matrices==6?unsigned(DL::Input):unsigned(DL::PostNormalized);
  descriptor[54]=RB::Projected;descriptor[55]=RB::PeerJobs;descriptor[56]=RB::Nonces;descriptor[57]=RB::PeerBoots;
  for(unsigned i=0;i<64;i++)assert(memory.at(RB::Header+i*4)==descriptor[i]);
  if(matrices==0){
   unsigned original=!CL::Dense(layer)?unsigned(RB::Original):unsigned(DL::Original),normalized=!CL::Dense(layer)?unsigned(RB::Input):unsigned(DL::Raw);
   for(unsigned i=0;i<5120;i++)assert(memory.at(original+i*4)==input[i] && memory.at(RB::OriginalCopy+i*4)==input[i]);
   Rms(original,normalized,0);
  }
  if((!CL::Dense(layer) && matrices==5) || (CL::Dense(layer) && matrices==4))Rms(RB::PostResidual,RB::PostNormalized,1);
  if(matrices==(!CL::Dense(layer)?7U:6U)){
   double delta=0,norm=0;unsigned gate=!CL::Dense(layer)?PioneerQ38MixedRecurrentOffset(2):unsigned(DL::GateOffset),up=!CL::Dense(layer)?PioneerQ38MixedRecurrentOffset(3):unsigned(DL::UpOffset);
   for(unsigned i=0;i<17408;i++){
    float a=Float(Get(outer[8],outer[9]+gate+i*4)),b=Float(Get(outer[8],outer[9]+up+i*4));double e=exp(-fabs(double(a))),sigmoid=a>=0?1/(1+e):e/(1+e);
    float expected=(a*float(sigmoid))*b,actual=Float(memory.at(RB::Input+i*4));assert(isfinite(actual));delta+=(double(actual)-expected)*(double(actual)-expected);norm+=double(expected)*expected;
   }
   assert(sqrt(delta/(norm?norm:1))<4e-6);activationChecks++;
  }
  for(unsigned at=0;at<r[14];at+=256){
   float raw[256];PioneerQ38ProjectionInput q;for(unsigned i=0;i<256;i++)raw[i]=Float(memory.at(descriptor[53]+(at+i)*4));ProjectionInputReference(r[15],raw,q);
   for(unsigned i=0;i<128;i++)assert(memory.at(RB::Native+(at/2+i)*4)==q.operand[i]);
   unsigned n=r[15]==8?8:1,first=at>>(r[15]==8?5:8);for(unsigned i=0;i<n;i++)assert(memory.at(RB::Scales+(first+i)*4)==q.scale[i]);
  }
  quantChecks++;matrices++;
  printf("OBSERVED connected layer%u matrix%u role%u/actual quantized input/same workers\n",layer,matrices,r[43]);
 }
 void VerifyLayer(){
  assert(completed==layer && matrices==(!CL::Dense(layer)?8U:7U) && finalWords==5120 && inputReads==5120);
  if(!CL::Dense(layer)){
   namespace H=PioneerQ38RecurrentHistoryLayout;RetainedState saved;
   saved.route=outer[8];assert(H::Address(outer[9],layer,saved.history));
   saved.convolution.resize(H::Bytes/4);saved.state.resize(96*8192);
   for(unsigned i=0;i<H::Bytes/4;i++){
    saved.convolution[i]=Get(saved.route,saved.history+i*4);
    assert(saved.convolution[i]==Get(saved.route,outer[9]+TT_Q38_CONV_NEXT+i*4));
   }
   for(unsigned w=0;w<96;w++){
    const auto* st=recurrent[layer]->Gdn()+w*192+64;saved.next[w]={st[13],st[7]};
    for(unsigned i=0;i<8192;i++){
     assert(Get(st[12],st[5]+i*4)==0); // current slot remains the original zero RAW input
     uint32_t value=Get(st[13],st[7]+i*4);assert(isfinite(Float(value)));saved.state[w*8192+i]=value;
    }
   }
   retained.push_back(std::move(saved)); // observer output ONLY, never fed back
  }
  for(unsigned i=0;i<5120;i++)input[i]=Get(outer[8],outer[9]+RB::FinalOffset+i*4);
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];Advance(w);unsigned delta=!CL::Dense(layer)?PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w):DL::RunDelta(w);
   assert(!p.active && p.GdnFinished(p.sequence) && p.sequence==nonceBefore[w]+delta && memory.at(RB::Nonces+w*4)==p.sequence);
   nonceBefore[w]=p.sequence;
  }
  completed++;printf("PASS ALL64 connected layer%u/final5120/ALL119 genuine closed chains/actual on-card hidden for next layer/ROOTinstructions%llu/reservedStack%u/writtenStack%u\n",layer,(unsigned long long)observation.Total(steps),0x70000-minSp,0x70000-minStack);
  // This callback follows actual final5120, input, numeric and all119 closed
  // chain checks above. Rebase ONLY observation bookkeeping. Keep low8 so
  // worker Tick ordering,64bit clock,NoC counters and every processor/private
  // datum remain exactly as before. No emulated instruction is skipped.
  assert(observation.RebaseAfterVerifiedLayer(steps,completed));
  profile.Dump(observation.Total(steps));
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && !CodeAddress(a));
  if(Range(a,0xffb20000,40)){assert(!pending);command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(memory.count(a));if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(a==RB::Status+4 && value==layer+1)VerifyLayer();
  if(a==RB::Doorbell){
   releases++;assert(value==releases && value>=1 && value<=65);
   if(value<=64){
    assert(completed==value-1);layer=value-1;matrices=lastMatrix=finalWords=inputReads=0;
    for(unsigned i=0;i<128;i++){outer[i]=memory.at(RB::Outer+i*4);assert(outer[i]==packages[layer]->Outer()[i]);}
   }else{
    // The non-returning final entry does not publish a returning-layer
    // status. Its own emitted next doorbell follows real final arithmetic
    // and all private frame checks, before the actual STOP audit.
    VerifyLayer();
    uint32_t stop[128];for(unsigned i=0;i<128;i++)stop[i]=memory.at(RB::Outer+i*4);
    assert(completed==64 && DL::StopShape(stop) && stop[48]==65);
   }
  }
  if(a==RB::Status+20 && !CL::Dense(layer) && value==7 && memory.at(a)!=7){
   for(unsigned w=0;w<120;w++)for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++)for(unsigned c=0;c<16;c++){
    const auto* d=recurrent[layer]->Conv()+w*384+j*64;float sum=0;
    for(unsigned t=0;t<4;t++)sum+=Float(t==3?Get(d[8],d[4]+d[45]+c*4):Get(d[41],d[15]+(c*3+t)*4))*Float(Get(d[13],d[12]+(c*4+t)*4));
    double expected=double(sum)/(1+exp(-double(sum))),actual=Float(Get(d[8],d[4]+d[43]+c*4));assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected)));
   }
   for(unsigned w=0;w<96;w++){Advance(w);auto& p=*child[w];assert(!p.active);p.VerifyGdn(p.sequence);}
   gdnChecks++;
  }
  if(a==RB::Status+20 && CL::Dense(layer) && value==6 && memory.at(a)!=6){
   double delta=0,norm=0;
   for(unsigned g=0;g<4;g++)for(unsigned h=0;h<6;h++)for(unsigned i=0;i<256;i++){
    uint32_t raw=memory.at(DL::NewV+g*512+(i/2)*4);uint16_t half=uint16_t(raw>>((i%2)*16));_Float16 hv;memcpy(&hv,&half,2);
    float gate=Float(memory.at(DL::Gate+(g*1536+h*256+i)*4));double e=exp(-fabs(double(gate))),sigmoid=gate>=0?1/(1+e):e/(1+e);
    float expected=float(hv)*float(sigmoid),actual=Float(memory.at(DL::Output+(g*1536+h*256+i)*4));assert(isfinite(actual));delta+=(double(actual)-expected)*(double(actual)-expected);norm+=double(expected)*expected;
   }
   assert(sqrt(delta/(norm?norm:1))<4e-6);attentionChecks++;
  }
  memory[a]=value;
 }
 void Submit(){
  packets++;bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],route=write?command[5]:command[2],bytes=command[8];
  assert(bytes && bytes<=16384 && !(bytes&3) && !command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
  bool bank=false;for(unsigned b=0;b<7;b++)bank |= route==PioneerQ38ProjectionRoute(b);
  if(bank){
   bool rootConv=Range(pc,0x64000,unsigned(conv.size()));assert(rootConv?Range(local,0x80000,0x1100):local==RB::Scratch);
   if(!write && remote==(!CL::Dense(layer)?outer[69]:outer[74])+inputReads*4 && (Phase()==1 || Phase()==2)){
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
     unsigned op=p.memory.at(M::Job+8);if(op==1)matrixJobs++;else if(op==7)convJobs++;else if(op==8)gdnJobs++;else if(op>=9 && op<=11)denseJobs++;else {assert(op==2 && completed==64);stopJobs++;}
     p.Receive();
    }
    peerWrites++;
   }else if(remote==0xffb121b0){assert(bytes==4);memory[local]=0x46800;}
   else if(remote==0xffb13140){assert(bytes==4);memory[local]=p.pc;}
   else if(Range(remote,0xffb20000,0x2000)){
    assert(bytes==4);memory[local]=p.Load(remote);
   }else if(remote==0x83f00 && bytes>256){
    assert(!p.active && p.memory.at(M::Job+8)==1 && p.CorrectRows() && !p.dotProbeReads && p.requests==p.packets.size());
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=i<64?p.memory.at(remote+i*4):p.buffers[4096+i-64];
   }else for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)>>6;counts[write?7:3]++;counts[2]++;counts[6]++;
 }
 void ObservedStep(){
  const uint64_t before=observation.Total(steps);profile.Record(pc);
  StepScalar();assert(observation.Total(steps)==before+1 && profile.Total()==before+1);
 }
 bool Run(){
  unsigned previousPC=0;
  while(pc!=0x60000 && pc!=0x60004){
   uint64_t before=observation.Total(steps);assert(before<PioneerQ38InstructionWindows::TotalLimit);
   previousPC=pc;cycles+=19;ObservedStep();
   if(!(steps&255))for(unsigned w=0;w<119;w++)Advance(w,64);
  }
  if(pc==0x60000)fprintf(stderr,"CHAIN_FAULT layer%u completed%u phase%u matrices%u jobs%u Conv%u GDN%u dense%u packets%u SP%x\n",layer,completed,Phase(),matrices,matrixJobs,convJobs,gdnJobs,denseJobs,packets,regs[2]);
  if(pc==0x60000)PioneerQ38InstructionFault::Dump(*this,previousPC,RB::Projected,RB::PostResidual,RB::Final,RB::Original);
  return pc==0x60004;
 }
 void Verify(){
  assert(pc==0x60004 && completed==64 && releases==65 && matrixJobs==46128 && convJobs==5712 && gdnJobs==4608 && denseJobs==4608 && stopJobs==119
   && rmsChecks==128 && quantChecks==496 && activationChecks==64 && gdnChecks==48 && attentionChecks==16 && minStack>=0x6e000 && minSp>=0x6e000 && retained.size()==48 && observation.Closed()==64);
  for(const auto& saved:retained){
   for(unsigned i=0;i<saved.convolution.size();i++)assert(Get(saved.route,saved.history+i*4)==saved.convolution[i]);
   for(unsigned w=0;w<96;w++)for(unsigned i=0;i<8192;i++)assert(Get(saved.next[w][0],saved.next[w][1]+i*4)==saved.state[w*8192+i]);
  }
  assert(memory.at(DL::StopSummary)==DL::StopMagic && memory.at(DL::StopSummary+4)==65 && memory.at(DL::StopSummary+8)==64 && memory.at(DL::StopSummary+12)==119);
  for(unsigned w=0;w<119;w++){
   const auto& p=*child[w];assert(p.pc==p.stopPC && p.memory.at(M::Status+4)==nonceBefore[w]+1 && !p.memory.at(M::Status+8) && p.memory.at(M::Status+16)==2);
  }
  unsigned before=packets;for(unsigned i=0;i<2048;i++)ObservedStep();assert(packets==before && pc==0x60004);
  profile.Dump(observation.Total(steps));
  printf("PASS ACTUAL ALL64 connected layers/ONE ROOT+SAME119+ONE byte fabric/496matrices46128jobs+Conv+GDN+attention/final5120/all48 provisional Conv+GDN NEXT retained EXACT/all119 REAL STOP/inert2048/combined ROOT reserved stack%u/written%u/8192;SYNTHETIC raw bytes,NOT original/native/physical/multi-token-state/token/Work/TPS\n",0x70000-minSp,0x70000-minStack);
 }
};
#ifndef PIONEER_Q38_LAYER_CHAIN64_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!strcmp(argv[3],"--refusals") || !strcmp(argv[3],"--prepare-only"))));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START ACTUAL ALL64 QCH1 instructions/SAME119 genuinely virgin peers/ONE GDDR map/ONLY raw learned+initial hidden+zero prior state;NO CPU products or worker state transplant");
 auto m=std::make_unique<ChainInstruction64Machine>(argv[1],argv[2]);
 if(argc==4 && !strcmp(argv[3],"--prepare-only")){
  assert(!m->steps && !m->packets && !m->peerWrites && !m->completed && m->spans.size());
  m->VerifyRawZeros();
  for(unsigned layer=0;layer<64;layer++)assert(m->packages[layer]->Matches(*m->canonical));
  for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));
  puts("PASS ALL64 instruction fixture RAW preparation/64canonical packages/SAME119 genuinely booted virgin workers/ONE fabric/explicit bounded disjoint raw-zero spans with actual writes taking priority/no products;ROOT instructions NOT executed/NO connected,STOP,native,physical,Work or TPS proof");return 0;
 }
 if(argc==4){
  for(unsigned bad=0;bad<12;bad++){
   fflush(nullptr);pid_t pid=fork();assert(pid>=0);
   if(!pid){
    if(bad==0)m->memory[RB::Boot]^=1;if(bad==1)m->memory[RB::Boot+40*4]^=1;if(bad==2)m->memory[RB::Boot+63*4]^=1;
    if(bad==3)m->memory[RB::Outer]^=1;if(bad==4)m->memory[RB::Outer+3*4]^=1;if(bad==5)m->memory[RB::Outer+6*4]^=1;
    if(bad==6)m->memory[RB::Outer+48*4]^=1;if(bad==7)m->memory[RB::Outer+32*4]^=1;if(bad==8)m->memory[RB::Outer+69*4]^=4;
    if(bad==9)m->source[0]^=1;if(bad==10)m->source[11]^=1;if(bad==11)m->memory[RB::Doorbell]=2;
    assert(!m->Run() && m->pc==0x60000 && !m->packets && !m->peerWrites && !m->completed);
    for(unsigned i=0;i<2048;i++)m->ObservedStep();assert(m->pc==0x60000 && !m->packets);
    for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));_exit(0);
   }
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS ACTUAL ALL64 scheduler initial12 boot+outer+source+doorbell guards/ZERO NoC+peer work/inert fault2048/SAME119 virgin state retained;NO positive connected/STOP/native/physical/TPS proof");return 0;
 }
 assert(m->Run());m->Verify();
}
#endif
