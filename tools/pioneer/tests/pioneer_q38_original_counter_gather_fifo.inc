/* SPDX-License-Identifier: MIT */
// Members of the original-byte fixture ONLY. Real captured4B commands and
// actual peer register bytes; never a numeric product/completion setter.
#if PIONEER_Q38_TEST_COUNTER_GATHER_FIFO != 1 || !defined(PIONEER_Q38_TEST_TID_FIFO_ENDPOINT) || PIONEER_Q38_TEST_TID_FIFO_ENDPOINT != 1
#error Exact original-byte queued FIFO TEST endpoint requires TID status model
#endif
using GatherCommand=std::array<uint32_t,10>;
std::vector<GatherCommand> gatherQueue,gatherReplies;
GatherCommand gatherInitiating={};
std::array<bool,10> gatherReadback={};
unsigned gatherStart=0,gatherEnd=0,gatherIssued=0,gatherOutstanding=0;
unsigned gatherBatches=0,gatherCaptured=0,gatherDelivered=0,gatherMaximumQueued=0,gatherMaximumOutstanding=0;
bool gatherActive=false;
void InitializeCounterGatherEndpoint(){
 assert(!steps && !packets && !gatherStart);
 const char* profile=getenv("PIONEER_TEST_ROOT_PROFILE_MAP");assert(profile);
 std::ifstream file(profile);assert(file.good());std::string line;
 while(std::getline(file,line)){
  std::istringstream in(line);unsigned start,size;std::string name;
  in>>std::hex>>start>>size;std::getline(in,name);
  if(name.find("PioneerQ38CounterGather::Read(")!=std::string::npos){
   assert(!gatherStart && start>=0x40000 && size && size<=0x60000-start);
   gatherStart=start;gatherEnd=start+size;
  }
 }
 assert(gatherStart && gatherEnd>gatherStart);
}
bool InCounterGather()const{return pc>=gatherStart && pc<gatherEnd;}
static unsigned GatherOffset(unsigned i){const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};assert(i<8);return at[i];}
void CaptureCounterGather(){
 assert(original.Ready() && InCounterGather() && (source[0]&0x10000U) && !pending && gatherIssued<8 && gatherQueue.size()<16);
 const unsigned remote=0xffb20000U+GatherOffset(gatherIssued);
 unsigned w=0;while(w<119 && M::Peer(w+1)!=command[2])w++;assert(w<119);
 const GatherCommand expected={remote,0,command[2],RB::Scratch+gatherIssued*32+(remote&31),0,129,0,0,4,0};
 assert(command==expected && memory.count(command[3]));
 for(bool read:gatherReadback)assert(read);
 if(!gatherQueue.empty())assert(gatherQueue.front()[2]==command[2]);
 gatherQueue.push_back(command);gatherIssued++;gatherCaptured++;packets++;
 gatherMaximumQueued=std::max(gatherMaximumQueued,unsigned(gatherQueue.size()));
 gatherReadback.fill(false);
}
void AdvanceCounterGather(){
 // Deliberately hold all eight captures before initiation. This exercises
 // queued-but-not-counted requests in the actual original instruction path.
 if(gatherIssued!=8)return;
 if(gatherActive){
  counts[2]++;counts[6]++;counts[3]++;gatherReplies.push_back(gatherInitiating);gatherActive=false;return;
 }
 if(!gatherQueue.empty()){
  gatherInitiating=gatherQueue.front();gatherQueue.erase(gatherQueue.begin());gatherActive=true;
  gatherOutstanding++;gatherMaximumOutstanding=std::max(gatherMaximumOutstanding,gatherOutstanding);return;
 }
 if(gatherReplies.empty())return;
 // Reverse reply order. Captured address/route/data, not submission order,
 // determines the destination;every response transports an actual CSR word.
 const auto request=gatherReplies.back();gatherReplies.pop_back();
 unsigned w=0;while(w<119 && M::Peer(w+1)!=request[2])w++;assert(w<119);Advance(w);
 assert(!child[w]->active);
 memory.at(request[3])=child[w]->Load(request[0]);
 counts[0]++;counts[1]++;gatherOutstanding--;gatherDelivered++;
 if(gatherReplies.empty()){
  assert(!gatherOutstanding && !gatherActive && gatherQueue.empty());
  gatherBatches++;gatherIssued=0;
 }
}
void VerifyCounterGatherEndpoint()const{
 assert(gatherBatches && gatherCaptured==gatherBatches*8 && gatherDelivered==gatherCaptured
  && !gatherIssued && !gatherOutstanding && !gatherActive && gatherQueue.empty() && gatherReplies.empty()
  && gatherMaximumQueued==8 && gatherMaximumOutstanding==8);
 printf("PASS actual original-byte FIFO endpoint/%u full8-read batches/%u captured+transported4B CSR replies/"
  "8 queued-before-counted and8 simultaneous outstanding/reverse response order/actual peer bytes+all master deltas/"
  "NO supplied products,worker completion,PC skip,native ownership,device timing or Work proof\n",gatherBatches,gatherDelivered);
}
