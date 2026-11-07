/* SPDX-License-Identifier: MIT */
// Actual emitted returning helper + actual ROOT packet seal. Synthetic
// counters/endpoints only; no model products, native ownership or card IO.
#define PIONEER_Q38_PEER_SOURCE_INSTRUCTION_MACHINE_ONLY
#include "pioneer_q38_peer_source_instruction_test.cpp"
#include <deque>
struct GatherMachine:AuditMachine {
 enum {Output=0x6e500};
 using Command=std::array<uint32_t,10>;
 std::deque<Command> queued;
 std::vector<Command> inflight;
 Command initiating={};
 std::array<bool,10> readback={};
 std::array<uint32_t,8> original={};
 unsigned submitted=0,started=0,responded=0,maximumQueued=0,maximumOutstanding=0;
 unsigned outstanding=0,ticks=0,delay=1,order=0,minimum=0x70000;
 unsigned badReadPacket=UINT32_MAX,badReadWord=0,badPostWord=UINT32_MAX;
 unsigned badSourcePacket=UINT32_MAX,badSource=0,badCounter=UINT32_MAX;
 unsigned postTid=UINT32_MAX,postOverflow=0,outputWrites=0,trackedWrites=0,faultWrites=0;
 bool active=false,drainedReturned=false,neverInitiate=false,dropResponse=false;
 bool fifoEnabled=true;
 std::vector<unsigned> responseOrder;
 GatherMachine(const AuditImage& rom,unsigned w):AuditMachine(rom,0,w){
  memory[Owner+1024]=2;
  memory[Owner+16*4]|=0x10000U;
  memory[0xffb20100]=memory.at(Owner+16*4);
  memory[Owner+768+16*4]=memory.at(Owner+16*4);
  memory[R::Boot+16*4]=memory.at(Owner+16*4);
  memory[R::Header+16*4]=memory.at(Owner+16*4);
  memory[0xffb20064]=0x10101010U;memory[0xffb20068]=0;
  for(unsigned i=0;i<8;i++){
   original[i]=counters[i];memory[Output+i*4]=0x7fc00001U;
   // Equal physical aliases2/6;otherwise independently distinguishable.
   peer[0xffb20000U+Counter(i)]=0x76500000U+(i==6?2:i)*0x111U;
  }
  regs[10]=Owner+768;regs[11]=Tracked;regs[12]=w;regs[13]=Output;
  for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U})regs[i]=0xa0000000U+i;
 }
 unsigned StepLimit()const override{return 40000000;}
 bool IsCounter(unsigned a)const{
  for(unsigned i=0;i<8;i++)if(a==0xffb20000U+Counter(i))return true;
  return a>=0xffb20240U && a<0xffb202c0U;
 }
 void Advance(){
  if(submitted!=8 || ++ticks%delay || neverInitiate)return;
  if(active){
   counters[2]++;counters[6]++;counters[3]++;
   inflight.push_back(initiating);active=false;started++;
   return;
  }
  if(!queued.empty()){
   initiating=queued.front();queued.pop_front();active=true;outstanding++;
   maximumOutstanding=std::max(maximumOutstanding,outstanding);return;
  }
  if(inflight.empty() || (dropResponse && inflight.size()==1))return;
  unsigned which=order==1?unsigned(inflight.size()-1):order==2?(responded*3)%unsigned(inflight.size()):0;
  const auto request=inflight[which];inflight.erase(inflight.begin()+which);
  // The actual requested register is read at response time. Its four
  // bytes are transported to the captured return address, not supplied
  // directly to out[] and not inferred from expected/retained counters.
  assert(peer.count(request[0]));memory[request[3]]=peer.at(request[0]);
  responseOrder.push_back((request[3]-P::Scratch)/32);
  counters[0]++;counters[1]++;outstanding--;responded++;
  if(responded==8 && badCounter<8){
   if(badCounter==2 || badCounter==6){counters[2]++;counters[6]++;}
   else counters[badCounter]++;
  }
 }
 uint32_t Load(unsigned at)override{
  assert(!(at&3));if(CodeAddress(at))return l1[Index(at)];
  if(at>=0xffb20000U && at<0xffb20028U){
   unsigned i=(at-0xffb20000U)/4;readback[i]=true;
   uint32_t v=command[i];
   if(submitted==badReadPacket && i==badReadWord)v^=1;
   if(responded==8 && i==badPostWord)v^=1;
   return v;
  }
  if(at==0xffb20040U || at==0xffb20840U || at==0xffb21040U || at==0xffb21840U
   || at==0xffb20064U || at==0xffb20068U)Advance();
  if(at==0xffb20040U)return active?1U:memory.at(at);
  if(at==0xffb20064U){
   unsigned result=memory.at(at);
   if(submitted)result=(result&0xffffff00U)|(16-unsigned(queued.size()));
   drainedReturned=result==0x10101010U;
   return result;
  }
  if(at==0xffb20068U)return memory.at(at)|(submitted==8?postOverflow:0);
  if(submitted && IsCounter(at)){
   // FIFO contents are invisible to the counters. Catch even a speculative
   // first counter load issued before the returned availability was tested.
   assert(drainedReturned && queued.empty());
  }
  if(at>=0xffb20240U && at<0xffb202c0U){
   unsigned index=(at-0xffb20240U)/4;
   return memory.at(at)+(index==0?outstanding:0)+(responded==8 && index==postTid?1U:0U);
  }
  for(unsigned i=0;i<16;i++)if(at==0xffb20000U+Source(i)
   && submitted==badSourcePacket && i==badSource)return memory.at(at)^1U;
  return AuditMachine::Load(at);
 }
 void Submit(){
  assert(fifoEnabled && submitted<8 && queued.size()<16);
  const unsigned remote=0xffb20000U+Counter(submitted);
  const Command expected={remote,0,PioneerQ38MixedWorkerLayout::Peer(worker+1),
   P::Scratch+submitted*32+(remote&31),0,129,0,0,4,0};
  assert(command==expected);
  for(bool seen:readback)assert(seen);
  queued.push_back(command);submitted++;maximumQueued=std::max(maximumQueued,unsigned(queued.size()));
  drainedReturned=false;readback.fill(false);
 }
 void Store(unsigned at,uint32_t value)override{
  assert(!(at&3));
  if(at==0x75460U || at==0x75448U){
   assert((faultWrites==0 && at==0x75460U && pc==image.entry[2]-24)
    || (faultWrites==1 && at==0x75448U && pc==image.entry[2]-8 && value==0x51465431U));
   memory[at]=value;faultWrites++;writes++;return;
  }
  if(at>=0xffb20000U && at<0xffb20028U){
   unsigned i=(at-0xffb20000U)/4;
   if(!i)readback.fill(false);
   command[i]=value;return;
  }
  if(at==0xffb20040U){assert(value==1);Submit();return;}
  const bool output=at>=Output && at<Output+32,tracked=at>=Tracked && at<Tracked+32;
  const bool stack=at>=0x6f000 && at<0x70000;
  assert(output || tracked || stack); // never mode,counter,model,release/host writes
  if(output || tracked){assert(responded==8 && queued.empty() && !active && inflight.empty());}
  if(output)outputWrites++;
  if(tracked)trackedWrites++;
  if(stack)minimum=std::min(minimum,at);
  memory[at]=value;writes++;
 }
 bool Run(){
  while(pc!=Return && pc!=image.entry[2])Step();
  bool okay=pc==Return;
  if(okay){
   assert(submitted==8 && responded==8 && started==8 && maximumQueued==8 && maximumOutstanding==8);
   assert(queued.empty() && !active && inflight.empty() && !outstanding);
   assert(outputWrites==8 && trackedWrites==8 && regs[1]==Return && regs[2]==0x70000 && regs[27]==Owner);
   for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U})assert(regs[i]==0xa0000000U+i);
   for(unsigned i=0;i<8;i++){
    assert(memory.at(Output+i*4)==peer.at(0xffb20000U+Counter(i)));
    assert(memory.at(Tracked+i*4)==counters[i]);
    assert(counters[i]==uint32_t(original[i]+(i==0 || i==1 || i==2 || i==3 || i==6?8U:0U)));
   }
  }else{
   assert(pc==image.entry[2] && faultWrites==2 && memory.at(0x75448)==0x51465431U);
   assert(!outputWrites && !trackedWrites);
   for(unsigned i=0;i<8;i++)assert(memory.at(Tracked+i*4)==original[i] && memory.at(Output+i*4)==0x7fc00001U);
  }
  const auto before=memory;const unsigned commands=submitted,writeCount=writes,received=responded;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==(okay?Return:image.entry[2]) && memory==before && submitted==commands && writes==writeCount && responded==received);
  return okay;
 }
};
struct CounterSealMachine:GatherMachine {
 enum {Expected=0x6e600};
 unsigned published=0;
 CounterSealMachine(const AuditImage& rom,unsigned w):GatherMachine(rom,w){
  pc=image.entry[1];regs[10]=Tracked;regs[11]=w;regs[12]=Expected;
  for(unsigned i=0;i<8;i++){
   memory[Expected+i*4]=peer.at(0xffb20000U+Counter(i));
   memory[C::Counters+w*32+i*4]=0x7fc00001U;
  }
 }
 void Store(unsigned at,uint32_t value)override{
  if(at>=C::Counters+worker*32 && at<C::Counters+worker*32+32){
   assert(responded==8 && queued.empty() && !active && inflight.empty());
   assert(value==memory.at(Expected+at-(C::Counters+worker*32)));
   memory[at]=value;published++;writes++;return;
  }
  if(at==C::Summary+8*4 || at==C::Summary+9*4){memory[at]=value;writes++;return;}
  GatherMachine::Store(at,value);
 }
 bool RunSeal(){
  while(pc!=Return && pc!=image.entry[2])Step();bool okay=pc==Return;
  assert(submitted==8 && started==8 && responded==8 && !outstanding);
  assert(trackedWrites==8 && !outputWrites); // helper finishes before receipt comparison
  for(unsigned i=0;i<8;i++)assert(memory.at(Tracked+i*4)==counters[i]);
  if(okay){
   assert(published==8 && regs[1]==Return && regs[2]==0x70000 && regs[27]==Owner);
   for(unsigned i=0;i<8;i++)assert(memory.at(C::Counters+worker*32+i*4)==memory.at(Expected+i*4));
  }else{
   assert(pc==image.entry[2] && faultWrites==2 && !published);
  } // peer baseline is NEVER re-captured on mismatch
  const auto before=memory;const unsigned writeCount=writes,commands=submitted;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==(okay?Return:image.entry[2]) && memory==before && writes==writeCount && submitted==commands);
  return okay;
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);AuditImage image(argv[1],argv[2]);unsigned cases=0;
 for(unsigned worker:{0U,95U,118U})for(unsigned delay:{1U,3U,17U})for(unsigned order=0;order<3;order++){
  auto m=std::make_unique<GatherMachine>(image,worker);m->delay=delay;m->order=order;
  assert(m->Run());cases++;
  printf("PASS emitted gather worker%u delay%u order%u:8 captured4B requests,8 simultaneously outstanding,actual byte replies,all8 deltas incl wrap+alias,HOST instructions%u stack%u;NOT device cycles\n",
   worker,delay,order,m->steps-2048,0x70000-m->minimum);
 }
 for(unsigned i=0;i<38;i++)for(unsigned bit=0;bit<32;bit++){
  auto m=std::make_unique<GatherMachine>(image,0);
  unsigned at=i<4?0xffb20040U+i*0x800:i==4?0xffb20064U:i==5?0xffb20068U:0xffb20240U+(i-6)*4;
  m->memory.at(at)^=1U<<bit;
  bool ignored=i>=6 && bit>=8;
  assert(m->Run()==ignored);
  if(!ignored)assert(m->submitted==0);
  cases++;
 }
 for(unsigned packet=0;packet<8;packet++)for(unsigned word=0;word<10;word++){
  auto m=std::make_unique<GatherMachine>(image,0);m->badReadPacket=packet;m->badReadWord=word;
  assert(!m->Run() && m->submitted==packet);cases++;
 }
 for(unsigned packet=0;packet<8;packet++)for(unsigned source=0;source<16;source++){
  auto m=std::make_unique<GatherMachine>(image,0);m->badSourcePacket=packet;m->badSource=source;
  assert(!m->Run() && m->submitted==packet);cases++;
 }
 for(unsigned word=0;word<10;word++){
  auto m=std::make_unique<GatherMachine>(image,0);m->badPostWord=word;
  assert(!m->Run() && m->submitted==8 && m->responded==8);cases++;
 }
 for(unsigned bit=0;bit<4;bit++){
  auto m=std::make_unique<GatherMachine>(image,0);m->postOverflow=1U<<bit;
  assert(!m->Run() && m->submitted==8);cases++;
 }
 for(unsigned tid=0;tid<32;tid++){
  auto m=std::make_unique<GatherMachine>(image,0);m->postTid=tid;
  assert(!m->Run() && m->responded==8);cases++;
 }
 // Coherently disabled sealed mode: the actual guard itself must refuse,
 // rather than relying on mismatched metadata or a zero FIFO status.
 {
  auto m=std::make_unique<GatherMachine>(image,0);m->fifoEnabled=false;
  for(unsigned at:{unsigned(AuditMachine::Owner+16*4),unsigned(AuditMachine::Owner+768+16*4),
   unsigned(R::Boot+16*4),unsigned(R::Header+16*4),0xffb20100U})m->memory.at(at)&=~0x10000U;
  assert(!m->Run() && !m->submitted);cases++;
 }
 for(unsigned which=0;which<2;which++){
  auto m=std::make_unique<GatherMachine>(image,0);m->neverInitiate=which==0;m->dropResponse=which==1;
  assert(!m->Run() && m->submitted==8);cases++;
 }
 for(unsigned index=0;index<8;index++){
  auto m=std::make_unique<GatherMachine>(image,0);m->badCounter=index;
  assert(!m->Run() && m->responded==8);cases++;
 }
 for(unsigned worker:{0U,95U,118U}){
  auto m=std::make_unique<CounterSealMachine>(image,worker);assert(m->RunSeal());cases++;
 }
 for(unsigned i=0;i<8;i++)for(unsigned bit=0;bit<32;bit++){
  auto peerFault=std::make_unique<CounterSealMachine>(image,0);
  peerFault->peer.at(0xffb20000U+Counter(i))^=1U<<bit;
  assert(!peerFault->RunSeal());cases++;
  auto receiptFault=std::make_unique<CounterSealMachine>(image,0);
  receiptFault->memory.at(CounterSealMachine::Expected+i*4)^=1U<<bit;
  assert(!receiptFault->RunSeal());cases++;
 }
 printf("PASS emitted counter gather/%u synthetic cases;actual packet seal,all4 initiators+FIFO-before-counter,all32 TIDs,16 sources per request,"
  "command pre/post readbacks,queued-before-counted+reverse replies,finite lost-request/response/counter timeouts,"
  "actual CounterSeal all8x32 physical/receipt corruptions refused without rebase,"
  "no mode/counter/worker/host writes,no output/frontier publication before complete,2048 inert steps EACH;"
  "NO original-model,native,hardware,Work or TPS qualification\n",cases);
 return 0;
}
