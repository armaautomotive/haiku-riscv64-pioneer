/* SPDX-License-Identifier: MIT */
// MOCK config endpoints ONLY. The compiled frontend performs every access.
#if !defined(PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_TEST) || PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_TEST != 1
#error Explicit offline prefetch worker observer required
#endif
struct InstructionPrefetchPeer:PreparedQ4Peer {
 struct Event{char kind;unsigned index,value;};
 struct ReachedFault{};
 std::array<Event,18> golden;
 unsigned prefetchEvents=0,beforeControl=0,beforeLimit=0,mainFirst=0,mainEnd=0;
 unsigned corruptRead=0,corruptWrite=0,parkEvents=UINT32_MAX;
 bool throwOnFault=false;
 InstructionPrefetchPeer(const char* fixture,const char* image,unsigned type,unsigned k,unsigned groups,unsigned prefix=0,unsigned worker=69,unsigned route=0x47,
  unsigned control=0,unsigned limit=0):PreparedQ4Peer(fixture,image,type,k,groups,prefix,worker,route),beforeControl(control),beforeLimit(limit){
  config[203]=limit;config[208]=control;
  const unsigned selected=(control&~0x1e1U)|0x81U;
  golden={{{'R',203,limit},{'R',208,control},{'R',203,limit},{'R',208,control},
   {'W',208,control&~1U},{'R',208,control&~1U},{'W',203,0xc0c67},{'R',203,0xc0c67},
   {'W',208,selected},{'R',208,selected},{'R',203,0xc0c67},{'R',208,selected},
   {'W',208,selected&~1U},{'R',208,selected&~1U},{'W',203,limit},{'R',203,limit},
   {'W',208,control},{'R',208,control}}};
  std::ifstream functions(std::string(image)+"/program.functions");assert(functions);
  std::string name,kind,first,size;
  while(functions>>name>>kind>>first>>size)if(name=="q38_global_worker_main"){
   assert(!mainFirst && kind=="FUNC");mainFirst=unsigned(std::stoul(first,nullptr,16));mainEnd=mainFirst+unsigned(std::stoul(size,nullptr,10));
  }
  assert(mainFirst==0x21128 && mainFirst<mainEnd);
 }
 void TouchPrefetch(char kind,unsigned index,unsigned value){
  assert(prefetchEvents<18 && pc>=mainFirst && pc<mainEnd && !exact && !matrixScope
   && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !recordRemaining);
  const auto e=golden.at(prefetchEvents++);assert(e.kind==kind && e.index==index);
  assert(value==(e.value^unsigned(kind=='R' && corruptWrite && prefetchEvents==corruptWrite+1)));
 }
 unsigned Load(unsigned at)override{
  if(at!=0xffef032c && at!=0xffef0340)return PreparedQ4Peer::Load(at);
  const unsigned index=(at-0xffef0000)/4,value=config[index];TouchPrefetch('R',index,value);
  return value^unsigned(prefetchEvents==corruptRead);
 }
 void Store(unsigned at,unsigned value)override{
  if(at!=0xffef032c && at!=0xffef0340){PreparedQ4Peer::Store(at,value);return;}
  const unsigned index=(at-0xffef0000)/4;TouchPrefetch('W',index,value);
  config[index]=value^unsigned(prefetchEvents==corruptWrite);configWrites++;
 }
 void StepBlock()override{
  if(pc==stopPC || pc==faultPC){
   if(parkEvents==UINT32_MAX)parkEvents=prefetchEvents;else assert(parkEvents==prefetchEvents);
  }
  PreparedQ4Peer::StepBlock();
  if(pc==stopPC)assert(prefetchEvents==18 && config[203]==beforeLimit && config[208]==beforeControl);
  if(pc==faultPC && throwOnFault)throw ReachedFault{};
 }
};
