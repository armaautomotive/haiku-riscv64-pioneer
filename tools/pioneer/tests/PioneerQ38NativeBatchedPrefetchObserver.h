/* SPDX-License-Identifier: MIT */
// HOST raw-byte endpoint/observer ONLY. No CPU products,guest PC/register,
// ready markers,completion proofs or native-owner state are supplied.
#if !defined(PIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST) || PIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST) || PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE) || PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY) \
 || defined(PIONEER_Q38_NATIVE_PEER_PREFETCH_TEST) || defined(_KERNEL_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact separate HOST batched-prefetch endpoint observer required
#endif
// TT_PIONEER_Q38_WORK_MODE is inherited by the HOST ISA model for the
// worker's arithmetic definitions,as in the unchanged peer model. It is
// not a native owner or a runtime Assistant Work binding. The separate
// immutable expected-ROM DATA TU must still reject that flag.
template<class Base> struct NativeBatchedPrefetchPeer:Base {
 using Base::Range;using Base::memory;using Base::packets;using Base::tiles;using Base::tile;
 using Base::pc;using Base::regs;using Base::inputs;using Base::type;using Base::requests;using Base::responses;
 using Base::pending;using Base::inputReads;using Base::matrixInstructions;using Base::rawPending;
 using Base::phaseQueue;using Base::pendingAccumulator;using Base::command;using Base::readback;
 std::vector<uint8_t> fetch;
 bool prefetchMatrix=false,prefetchRestoring=false;
 unsigned prefetchStages=0,prefetchRestores=0,prefetchJobsVerified=0,prefetchPendingOps=0;
 unsigned prefetchCopyReads=0,prefetchCopyWrites=0;
 uint32_t prefetchReturn=0;mutable unsigned prefetchValidated=UINT32_MAX;
 NativeBatchedPrefetchPeer(const char* old,const char* image,unsigned worker):Base(old,image,worker),
  fetch(Read((std::string(image)+"/fetch.bin").c_str())){
  assert(!Base::steps && pc==0x6000 && fetch.size()==1260);
  Base::delay=3; // endpoint reply delay,NOT hardware latency or throughput
 }
 const std::vector<uint8_t>* Rom(unsigned at,unsigned& base)const override {
  if(Range(at,0x70000,unsigned(fetch.size()))){base=0x70000;return &fetch;}
  return Base::Rom(at,base);
 }
 unsigned PrefetchBatch()const{return type==8?4:2;}
 unsigned PrefetchBlocks()const{return inputs/(type==8?32:256);}
 unsigned PrefetchPhases()const{return Base::MatrixPhasesPerPanel()*(type==8?2:16);}
 uint32_t PrefetchRaw(unsigned t)const{return t%PrefetchBatch()==PrefetchBatch()-1?0x90000:0x98000+(t%PrefetchBatch())*unsigned(tile.size());}
 unsigned PrefetchTile()const {
  assert(prefetchMatrix && prefetchStages && prefetchStages<=tiles.size());return prefetchStages-1;
 }
 bool PrefetchRawRange(unsigned at)const{return Range(at,0x90000,unsigned(tile.size())) || Range(at,0x98000,PrefetchBatch()*unsigned(tile.size()));}
 uint32_t RawInputBase()const override {
  if(!prefetchMatrix)return Base::RawInputBase();
  unsigned t=PrefetchTile();uint32_t raw=PrefetchRaw(t);
  assert(responses>=inputReads+t/PrefetchBatch()+1 && (!pending || raw==0x90000));
  if(prefetchValidated!=t){
   for(unsigned w=0;w<tile.size()/4;w++)assert(memory.at(raw+w*4)==Word(tiles.at(t),w*4));prefetchValidated=t;
  }
  return raw;
 }
 bool DisjointPrefetchCompute()const override {
  if(!prefetchMatrix || !pending || !prefetchStages || prefetchReturn || requests<=inputReads)return false;
  unsigned t=PrefetchTile();const auto& p=packets.at(requests-1);
  if(t%PrefetchBatch()!=PrefetchBatch()-1 || t+1>=tiles.size() || (t+1)%PrefetchBlocks()==0
   || p.tile!=int(t+1) || p.destination!=0x98000 || p.bytes!=PrefetchBatch()*tile.size()
   || requests!=inputReads+(t+1)/PrefetchBatch()+1 || responses!=requests-1
   || matrixInstructions<t*PrefetchPhases() || matrixInstructions>(t+1)*PrefetchPhases())return false;
  return RawInputBase()==0x90000;
 }
 void Instruction(uint32_t op)override {
  if(prefetchMatrix && pending){assert(DisjointPrefetchCompute());prefetchPendingOps++;}
  Base::Instruction(op);
 }
 void Receive(){
  prefetchMatrix=false;Base::Receive();unsigned op=memory.at(M::Job+8);
  if(op!=1 && op!=G::GlobalMatrix)return;
  assert(!pending && !prefetchReturn && !(PrefetchBlocks()%PrefetchBatch())
   && !(tiles.size()%PrefetchBatch()) && packets.size()==inputReads+tiles.size());
  prefetchStages=prefetchRestores=prefetchCopyReads=prefetchCopyWrites=0;
  prefetchValidated=UINT32_MAX;prefetchRestoring=false;
  auto original=packets;packets.resize(inputReads);
  for(unsigned t=0;t<tiles.size();t+=PrefetchBatch()){
   const auto& p=original.at(inputReads+t);
   packets.push_back({p.address,p.route,0x98000,PrefetchBatch()*unsigned(tile.size()),int(t)});
  }
  prefetchMatrix=true;
 } // Per-job endpoint bookkeeping ONLY;NEVER processor/tensor/native-state reset.
 uint32_t Load(uint32_t at)override {
  if(prefetchMatrix && PrefetchRawRange(at)){
   if(prefetchReturn && Range(pc,0x70000,unsigned(fetch.size())) && Range(at,0x98000,PrefetchBatch()*unsigned(tile.size()))){
    unsigned t=PrefetchTile(),w=prefetchCopyReads%(tile.size()/4);
    assert(!pending && Base::drained && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
     && t%PrefetchBatch()==PrefetchBatch()-1 && at==0x98000+(t%PrefetchBatch())*tile.size()+w*4
     && memory.at(at)==Word(tiles.at(t),w*4));prefetchCopyReads++;return memory.at(at);
   }
   uint32_t raw=RawInputBase();assert(!(at&3) && at>=raw && at-raw+4<=tile.size());return memory.at(at);
  }
  if(prefetchMatrix && at==0xffb20040){
   Base::doorbellRead=true;
   if(pending && ++Base::waits>Base::delay && requests!=Base::faultAt){
    const auto& p=packets.at(requests-1);
    for(unsigned at=0;at<p.bytes;at+=4)memory[p.destination+at]=p.tile<0?Base::root.at(p.address+at)
     :Word(tiles.at(unsigned(p.tile)+at/tile.size()),at%tile.size());
    pending=false;responses++;
   } // ONLY completion of an actual issued NoC read;raw input bytes,not math.
   return unsigned(pending);
  }
  return Base::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(prefetchMatrix && PrefetchRawRange(at)){
   unsigned t=PrefetchTile(),w=prefetchCopyWrites%(tile.size()/4);
   assert(prefetchReturn && Range(pc,0x70000,unsigned(fetch.size())) && t%PrefetchBatch()==PrefetchBatch()-1
    && !pending && Base::drained && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
    && prefetchCopyReads==prefetchCopyWrites+1 && at==0x90000+w*4 && value==Word(tiles.at(t),w*4));
   memory[at]=value;prefetchCopyWrites++;return;
  }
  if(prefetchMatrix && (Range(pc,0x72000,unsigned(Base::code.size())) || Range(pc,0x70000,unsigned(fetch.size()))) && Range(at,0xffb20000,40)){
   assert(!pending && !rawPending && phaseQueue.empty() && !(at&3));unsigned w=(at-0xffb20000)/4;
   assert(w==0 || w==2 || w==3 || w==8);
   if(!w){prefetchRestoring=prefetchStages && !(prefetchStages%PrefetchBlocks()) && requests>inputReads
     && !((requests-inputReads)%(PrefetchBlocks()/PrefetchBatch()))
     && value==Base::base+prefetchStages*unsigned(tile.size())-tile.size();if(prefetchRestoring)prefetchRestores++;}
   assert(prefetchRestoring || requests<packets.size());const auto& p=packets.at(prefetchRestoring?requests-1:requests);
   bool initial=requests==inputReads && !prefetchStages;
   unsigned expected=w==0?(prefetchRestoring?Base::base+prefetchStages*unsigned(tile.size())-tile.size():p.address)
    :w==2?p.route:w==3?(prefetchRestoring || initial?0x90000:p.destination):(prefetchRestoring || initial?unsigned(tile.size()):p.bytes);
   assert(value==expected);command[w]=value;readback[w]=false;return;
  }
  if(prefetchMatrix && at==0xffb20040 && !command[7])assert(!prefetchRestoring && !rawPending && phaseQueue.empty());
  if(prefetchMatrix && at==M::Status+4){
   assert(value==Base::sequence && requests==packets.size() && responses==requests && !pending && !rawPending
    && phaseQueue.empty() && pendingAccumulator.empty() && prefetchStages==tiles.size()
    && prefetchRestores==tiles.size()/PrefetchBlocks() && !prefetchReturn
    && prefetchCopyReads==prefetchCopyWrites && prefetchCopyWrites==tiles.size()/PrefetchBatch()*(tile.size()/4));
   prefetchJobsVerified++;
  }
  Base::Store(at,value);
 }
 void StepBlock()override {
  if(prefetchMatrix && pc==0x70194){assert(!prefetchReturn);prefetchReturn=regs[1];prefetchStages++;}
  Base::StepBlock();
  if(prefetchReturn && pc==prefetchReturn){prefetchReturn=0;assert(regs[10]==RawInputBase());}
 }
};
