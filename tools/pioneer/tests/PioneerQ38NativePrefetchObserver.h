/* SPDX-License-Identifier: MIT */
// HOST instruction observer ONLY. All operands come from real raw byte
// endpoints;no guest PC/register/result/completion or NIU state is supplied.
#if !defined(PIONEER_Q38_NATIVE_PEER_PREFETCH_TEST) || PIONEER_Q38_NATIVE_PEER_PREFETCH_TEST != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST) || PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)
#error Exact separate HOST prefetch instruction observer required
#endif
template<class Base> struct NativePrefetchPeer:Base {
 using Base::Range;using Base::memory;using Base::packets;using Base::tiles;using Base::tile;
 using Base::pc;using Base::regs;using Base::inputs;using Base::type;using Base::requests;using Base::responses;
 using Base::pending;using Base::inputReads;using Base::matrixInstructions;using Base::rawPending;
 using Base::phaseQueue;using Base::pendingAccumulator;using Base::command;using Base::readback;
 std::vector<uint8_t> fetch;
 bool prefetchMatrix=false,prefetchRestoring=false;
 unsigned prefetchStages=0,prefetchRestores=0,prefetchJobsVerified=0,prefetchPendingOps=0;
 uint32_t prefetchReturn=0;
 mutable unsigned prefetchValidated=UINT32_MAX;
 NativePrefetchPeer(const char* old,const char* image,unsigned worker):Base(old,image,worker),
  fetch(Read((std::string(image)+"/fetch.bin").c_str())){
  assert(!Base::steps && pc==0x6000 && fetch.size()==856);
  Base::delay=3; // Synthetic endpoint holds the reply beyond issue readback;
  // actual pending/counter/data semantics remain visible. NOT silicon time.
 }
 const std::vector<uint8_t>* Rom(unsigned at,unsigned& base)const override {
  if(Range(at,0x70000,unsigned(fetch.size()))){base=0x70000;return &fetch;}
  return Base::Rom(at,base);
 }
 unsigned PrefetchBlocks()const{return inputs/(type==8?32:256);}
 unsigned PrefetchPhases()const{return Base::MatrixPhasesPerPanel()*(type==8?2:16);}
 uint32_t PrefetchRaw(unsigned t)const{return ((t%PrefetchBlocks())&1)?0x98000:0x90000;}
 unsigned PrefetchTile()const {
  assert(prefetchMatrix && prefetchStages && !prefetchReturn && prefetchStages<=tiles.size());return prefetchStages-1;
 }
 uint32_t RawInputBase()const override {
  if(!prefetchMatrix)return Base::RawInputBase();
  unsigned t=PrefetchTile();uint32_t raw=PrefetchRaw(t);
  assert(responses>=inputReads+t+1 && (!pending || packets.at(requests-1).destination!=raw));
  if(prefetchValidated!=t){
   for(unsigned w=0;w<tile.size()/4;w++)assert(memory.at(raw+w*4)==Word(tiles.at(t),w*4));prefetchValidated=t;
  }
  return raw;
 }
 bool DisjointPrefetchCompute()const override {
  if(!prefetchMatrix || !pending || !prefetchStages || prefetchReturn || requests<=inputReads)return false;
  unsigned t=PrefetchTile();const auto& p=packets.at(requests-1);
  if(t+1>=tiles.size() || (t+1)%PrefetchBlocks()==0 || p.tile!=int(t+1) || p.destination!=PrefetchRaw(t+1)
   || p.bytes!=tile.size() || requests!=inputReads+t+2 || responses!=inputReads+t+1
   || matrixInstructions<t*PrefetchPhases() || matrixInstructions>(t+1)*PrefetchPhases())return false;
  return RawInputBase()!=p.destination;
 }
 void Instruction(uint32_t op)override {
  if(prefetchMatrix && pending){assert(DisjointPrefetchCompute());prefetchPendingOps++;}
  Base::Instruction(op);
 }
 void Receive() {
  prefetchMatrix=false;Base::Receive();
  unsigned op=memory.at(M::Job+8);
  if(op!=1 && op!=G::GlobalMatrix)return;
  assert(!pending && !prefetchReturn);prefetchStages=prefetchRestores=0;prefetchValidated=UINT32_MAX;prefetchRestoring=false;
  for(auto& p:packets)if(p.tile>=0)p.destination=PrefetchRaw(unsigned(p.tile));
  prefetchMatrix=true;
 } // Only per-job endpoint bookkeeping;NO processor/tensor/native state reset.
 uint32_t Load(uint32_t at)override {
  if(prefetchMatrix && (Range(at,0x90000,unsigned(tile.size())) || Range(at,0x98000,unsigned(tile.size())))){
   uint32_t raw=RawInputBase();assert(!(at&3) && at>=raw && at-raw+4<=tile.size());return memory.at(at);
  }
  return Base::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(prefetchMatrix && (Range(at,0x90000,unsigned(tile.size())) || Range(at,0x98000,unsigned(tile.size()))))assert(false);
  if(prefetchMatrix && (Range(pc,0x72000,unsigned(Base::code.size())) || Range(pc,0x70000,unsigned(fetch.size()))) && Range(at,0xffb20000,40)){
   assert(!pending && !rawPending && phaseQueue.empty() && !(at&3));unsigned w=(at-0xffb20000)/4;
   assert(w==0 || w==2 || w==3 || w==8);
   if(!w){prefetchRestoring=requests>inputReads && !((requests-inputReads)%PrefetchBlocks()) && value==packets.at(requests-1).address;if(prefetchRestoring)prefetchRestores++;}
   assert(prefetchRestoring || requests<packets.size());const auto& p=packets.at(prefetchRestoring?requests-1:requests);
   assert(value==(w==0?p.address:w==2?p.route:w==3?(prefetchRestoring?0x90000:p.destination):p.bytes));
   command[w]=value;readback[w]=false;return;
  }
  if(prefetchMatrix && at==0xffb20040 && !command[7])assert(!prefetchRestoring && !rawPending && phaseQueue.empty());
  if(prefetchMatrix && at==M::Status+4){
   assert(value==Base::sequence && requests==packets.size() && responses==requests && !pending && !rawPending
    && phaseQueue.empty() && pendingAccumulator.empty() && prefetchStages==tiles.size()
    && prefetchRestores==tiles.size()/PrefetchBlocks() && !prefetchReturn);prefetchJobsVerified++;
  }
  Base::Store(at,value);
 }
 void StepBlock()override {
  if(prefetchMatrix && pc==0x7018c){assert(!prefetchReturn);prefetchReturn=regs[1];prefetchStages++;}
  Base::StepBlock();
  if(prefetchReturn && pc==prefetchReturn){prefetchReturn=0;assert(regs[10]==RawInputBase());}
 }
};
