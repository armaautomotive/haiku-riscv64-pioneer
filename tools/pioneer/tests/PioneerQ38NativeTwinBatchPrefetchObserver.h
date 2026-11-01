/* SPDX-License-Identifier: MIT */
// HOST raw-byte endpoint/observer ONLY. Never supplies CPU products,
// processor PC/registers,ready/receipt markers or native-owner state.
#if !defined(PIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST) || PIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST) || PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE) || PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY) \
 || defined(PIONEER_Q38_NATIVE_PEER_PREFETCH_TEST) || defined(PIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST) \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Exact distinct HOST twin-batch endpoint observer required
#endif
// TT_PIONEER_Q38_WORK_MODE belongs to the unchanged HOST ISA arithmetic
// definitions ONLY. The immutable expected-ROM TU rejects that flag.
template<class Base> struct NativeTwinBatchPrefetchPeer:Base {
 using Base::Range;using Base::memory;using Base::packets;using Base::tiles;using Base::tile;
 using Base::pc;using Base::regs;using Base::inputs;using Base::type;using Base::requests;using Base::responses;
 using Base::pending;using Base::inputReads;using Base::matrixInstructions;using Base::rawPending;
 using Base::phaseQueue;using Base::pendingAccumulator;using Base::command;using Base::readback;
 std::vector<uint8_t> fetch;
 bool prefetchMatrix=false,prefetchRestoring=false;
 unsigned prefetchStages=0,prefetchRestores=0,prefetchJobsVerified=0,prefetchPendingOps=0;
 uint32_t prefetchReturn=0;mutable unsigned prefetchValidated=UINT32_MAX;
 NativeTwinBatchPrefetchPeer(const char* old,const char* image,unsigned worker):Base(old,image,worker),
  fetch(Read((std::string(image)+"/fetch.bin").c_str())){
  assert(!Base::steps && pc==0x6000 && fetch.size()==1292);Base::delay=3;
 } // endpoint reply delay is NOT hardware latency or a speed measurement
 const std::vector<uint8_t>* Rom(unsigned at,unsigned& base)const override {
  if(Range(at,0x70000,unsigned(fetch.size()))){base=0x70000;return &fetch;}
  return Base::Rom(at,base);
 }
 unsigned PrefetchBatch()const{return type==8?4:2;}
 unsigned PrefetchBlocks()const{return inputs/(type==8?32:256);}
 unsigned PrefetchPhases()const{return Base::MatrixPhasesPerPanel()*(type==8?2:16);}
 unsigned PrefetchBuffer(unsigned t)const{return ((t%PrefetchBlocks())/PrefetchBatch())%2?0x9a000:0x98000;}
 unsigned PrefetchTile()const {
  assert(prefetchMatrix && prefetchStages && prefetchStages<=tiles.size());return prefetchStages-1;
 }
 uint32_t PrefetchRaw(unsigned t)const{return PrefetchBuffer(t)+(t%PrefetchBatch())*unsigned(tile.size());}
 bool PrefetchRawRange(unsigned at)const{return Range(at,0x90000,unsigned(tile.size()))
  || Range(at,0x98000,PrefetchBatch()*unsigned(tile.size())) || Range(at,0x9a000,PrefetchBatch()*unsigned(tile.size()));}
 uint32_t RawInputBase()const override {
  if(!prefetchMatrix)return Base::RawInputBase();
  unsigned t=PrefetchTile();uint32_t raw=PrefetchRaw(t);
  assert(responses>=inputReads+t/PrefetchBatch()+1 && (!pending || packets.at(requests-1).destination!=PrefetchBuffer(t)));
  if(prefetchValidated!=t){
   for(unsigned w=0;w<tile.size()/4;w++)assert(memory.at(raw+w*4)==Word(tiles.at(t),w*4));prefetchValidated=t;
  }
  return raw;
 }
 bool DisjointPrefetchCompute()const override {
  if(!prefetchMatrix || !pending || !prefetchStages || prefetchReturn || requests<=inputReads)return false;
  unsigned t=PrefetchTile(),next=t-t%PrefetchBatch()+PrefetchBatch();const auto& p=packets.at(requests-1);
  return next<tiles.size() && next%PrefetchBlocks()!=0 && p.tile==int(next)
   && p.destination!=PrefetchBuffer(t) && p.destination==PrefetchBuffer(next) && p.bytes==PrefetchBatch()*tile.size()
   && requests==inputReads+next/PrefetchBatch()+1 && responses==requests-1
   && matrixInstructions>=t*PrefetchPhases() && matrixInstructions<=(t+1)*PrefetchPhases()
   && RawInputBase()==PrefetchRaw(t);
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
  prefetchStages=prefetchRestores=0;prefetchValidated=UINT32_MAX;prefetchRestoring=false;
  auto original=packets;packets.resize(inputReads);
  for(unsigned t=0;t<tiles.size();t+=PrefetchBatch()){
   const auto& p=original.at(inputReads+t);
   packets.push_back({p.address,p.route,PrefetchBuffer(t),PrefetchBatch()*unsigned(tile.size()),int(t)});
  }
  prefetchMatrix=true;
 } // per-job endpoint bookkeeping ONLY;no processor/tensor/native-state transplant
 uint32_t Load(uint32_t at)override {
  if(prefetchMatrix && PrefetchRawRange(at)){
   uint32_t raw=RawInputBase();assert(!(at&3) && at>=raw && at-raw+4<=tile.size());return memory.at(at);
  }
  if(prefetchMatrix && at==0xffb20040){
   Base::doorbellRead=true;
   if(pending && ++Base::waits>Base::delay && requests!=Base::faultAt){
    const auto& p=packets.at(requests-1);
    for(unsigned offset=0;offset<p.bytes;offset+=4)memory[p.destination+offset]=p.tile<0?Base::root.at(p.address+offset)
     :Word(tiles.at(unsigned(p.tile)+offset/tile.size()),offset%tile.size());
    pending=false;responses++;
   } // ONLY completes an actually issued NoC read with raw bytes,never products
   return unsigned(pending);
  }
  return Base::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(prefetchMatrix && PrefetchRawRange(at))assert(false && "copy-free native worker must never write raw-copy buffers");
  if(prefetchMatrix && (Range(pc,0x72000,unsigned(Base::code.size())) || Range(pc,0x70000,unsigned(fetch.size()))) && Range(at,0xffb20000,40)){
   assert(!pending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !(at&3));unsigned w=(at-0xffb20000)/4;
   assert(w==0 || w==2 || w==3 || w==8);
   unsigned last=prefetchStages?((PrefetchTile()/PrefetchBlocks()+1)*PrefetchBlocks()-1):0;
   if(!w){prefetchRestoring=prefetchStages && PrefetchTile()%PrefetchBlocks()==PrefetchBlocks()-PrefetchBatch()
     && value==Base::base+last*unsigned(tile.size());if(prefetchRestoring)prefetchRestores++;}
   assert(prefetchRestoring || requests<packets.size());const auto& p=packets.at(prefetchRestoring?requests-1:requests);
   bool initial=requests==inputReads && !prefetchStages;
   unsigned expected=w==0?(prefetchRestoring?Base::base+last*unsigned(tile.size()):p.address)
    :w==2?p.route:w==3?(prefetchRestoring || initial?0x90000:p.destination):(prefetchRestoring || initial?unsigned(tile.size()):p.bytes);
   assert(value==expected);command[w]=value;readback[w]=false;return;
  }
  if(prefetchMatrix && at==0xffb20040 && !command[7])assert(!prefetchRestoring && !rawPending && phaseQueue.empty() && pendingAccumulator.empty());
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
