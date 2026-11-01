/* SPDX-License-Identifier: MIT */
// Members of the SAME virgin native ROOT. HOST-only queued4B endpoint,
// actual installed instructions,actual peer CSR bytes;no worker products.
#if !defined(PIONEER_Q38_TOKEN_NATIVE_COUNTER_GATHER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_COUNTER_GATHER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS) || PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS != 1
#error Exact counter-gather native HOST fixture required
#endif
using GatherCommand=std::array<uint32_t,10>;
std::vector<GatherCommand> gatherQueue,gatherReplies;
GatherCommand gatherInitiating={};
std::array<bool,10> gatherReadback={};
unsigned gatherStart=0,gatherEnd=0,gatherIssued=0,gatherOutstanding=0;
unsigned gatherBatches=0,gatherCaptured=0,gatherDelivered=0,gatherMaximumQueued=0,gatherMaximumOutstanding=0;
bool gatherActive=false;
void InitializeCounterGatherEndpoint(){
 const char* range=getenv("PIONEER_TEST_COUNTER_GATHER_RANGE");assert(range);
 unsigned begin=0,end=0;char extra=0;
 assert(sscanf(range,"%x:%x%c",&begin,&end,&extra)==2 && begin>=0x40000 && end>begin && end<=0x60000 && !((begin|end)&3));
 uint32_t word=0;assert(PioneerQ38TokenPrograms::ProgramWord(0,begin,word)
  && PioneerQ38TokenPrograms::ProgramWord(0,end-4,word));
 gatherStart=begin;gatherEnd=end;
} // range comes from runner's SAME authenticated frozen ELF snapshot,not a PC/owner setter
bool InCounterGather()const{return pc>=gatherStart && pc<gatherEnd;}
static unsigned GatherOffset(unsigned i){const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};assert(i<8);return at[i];}
void CaptureCounterGather(){
 auto& root=io.Core(0);
 assert(original && original->Ready() && layerActive && fullLayerRunning && !layerHeld && InCounterGather()
  && !root.pending && (root.config&0x10000U) && gatherIssued<8 && gatherQueue.size()<16);
 GatherCommand captured;for(unsigned i=0;i<10;i++)captured[i]=root.command[i];
 const unsigned remote=0xffb20000U+GatherOffset(gatherIssued);
 unsigned worker=1;while(worker<120 && PioneerQ38SelectedGlobalWorker::Peer(worker)!=captured[2])worker++;
 assert(worker<120 && io.Core(worker).coreReset==0x46800);
 const GatherCommand expected={remote,0,captured[2],PioneerQ38GlobalChainLayout::R::Scratch+gatherIssued*32+(remote&31),0,129,0,0,4,0};
 assert(captured==expected && PioneerQ38TokenL1::SnapshotContains(0,captured[3]));
 for(bool read:gatherReadback)assert(read);
 if(!gatherQueue.empty())assert(gatherQueue.front()[2]==captured[2]);
 gatherQueue.push_back(captured);gatherIssued++;gatherCaptured++;submitted++;layerPackets++;
 root.doorbells++;root.doorbellRead=false;root.requests++;
 gatherMaximumQueued=std::max(gatherMaximumQueued,unsigned(gatherQueue.size()));gatherReadback.fill(false);
}
void AdvanceCounterGather(){
 if(gatherIssued!=8)return; // actual queued requests remain invisible to transaction/master counts
 auto& root=io.Core(0);assert(!root.pending);
 if(gatherActive){root.accepted++;root.counters[3]++;gatherReplies.push_back(gatherInitiating);gatherActive=false;return;}
 if(!gatherQueue.empty()){
  gatherInitiating=gatherQueue.front();gatherQueue.erase(gatherQueue.begin());gatherActive=true;
  gatherOutstanding++;gatherMaximumOutstanding=std::max(gatherMaximumOutstanding,gatherOutstanding);return;
 }
 if(gatherReplies.empty())return;
 const auto packet=gatherReplies.back();gatherReplies.pop_back();
 unsigned worker=1;while(worker<120 && PioneerQ38SelectedGlobalWorker::Peer(worker)!=packet[2])worker++;
 assert(worker<120 && io.Core(worker).coreReset==0x46800);
 // Real peer counters produced by its actual RISC/Tensix history. Only
 // this captured4B address is read and transported to its captured L1 slot.
 io.Raw(0,packet[3])=io.Core(worker).ResidentNocRead(packet[0]-0xffb20000U);
 root.counters[0]++;root.counters[1]++;gatherOutstanding--;gatherDelivered++;completed++;layerPeerReads++;
 if(gatherReplies.empty()){
  assert(!gatherOutstanding && !gatherActive && gatherQueue.empty());gatherBatches++;gatherIssued=0;
 }
}
void VerifyCounterGatherEndpoint()const{
 assert(gatherBatches==453 && gatherCaptured==gatherBatches*8 && gatherDelivered==gatherCaptured
  && gatherMaximumQueued==8 && gatherMaximumOutstanding==8 && !gatherIssued && !gatherOutstanding
  && !gatherActive && gatherQueue.empty() && gatherReplies.empty() && submitted==completed && !io.Core(0).pending);
 printf("PASS actual native original first-layer counter FIFO/%u batches/%u captured4B actual peer CSR replies/"
  "8 queued-before-counted+8 outstanding/reverse replies/real accepted+response deltas/no worker products or completion setter;"
  "NOT whole-model,hardware,Work or TPS proof\n",gatherBatches,gatherDelivered);
}
