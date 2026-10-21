/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
struct PeerPhaseImage:DirectSrcBImage{
 std::vector<uint8_t> helper;std::vector<std::pair<unsigned,uint32_t>> clocks;
 uint64_t epoch=UINT64_C(1)<<40;unsigned metricWrites=0;bool rollover=false,unstable=false,backward=false;
 PeerPhaseImage(const char* path,unsigned t,unsigned k,unsigned g):DirectSrcBImage(path,t,k,g){
  helper=Read((std::string(path)+"/fetch.bin").c_str());assert(!helper.empty()&&helper.size()<=0x700&&!(helper.size()&3));
  for(unsigned w=0;w<helper.size()/4;w++)l1[Index(0x70000+w*4)]=Word(helper,w*4);
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(helper.size()))||DirectSrcBImage::CodeAddress(at);}
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00||csr==0xb80);uint64_t now=epoch+uint64_t(steps)*19;
  if(backward&&clocks.size()>=15)now-=UINT64_C(1)<<39;
  uint32_t value=csr==0xb00?uint32_t(now):uint32_t(now>>32);
  if(rollover&&clocks.empty())value--;
  if(unstable&&csr==0xb80&&clocks.size()%3==2)value++;
  clocks.emplace_back(csr,value);return value;
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x70000,unsigned(helper.size())))return l1[Index(at)];
  return DirectSrcBImage::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,0x84500,64)){
   assert(Active()&&!pending&&!rawPending&&phaseQueue.empty()&&pendingAccumulator.empty()&&!(at&3));
   assert(matrixInstructions==groups*(inputs/(type==8?32:256))*(type==8?2:16)*MatrixPhasesPerPanel());
   memory[at]=value;metricWrites++;return;
  }
  DirectSrcBImage::Store(at,value);
 }
 void CheckMetrics(bool valid,unsigned sequence){
  assert(metricWrites==16&&memory.at(0x84500)==0x31545051&&memory.at(0x84504)==1&&memory.at(0x84508)==sequence
   &&memory.at(0x8450c)==logicalWorker&&memory.at(0x84510)==type&&memory.at(0x84514)==inputs&&memory.at(0x84518)==groups&&memory.at(0x8451c)==unsigned(valid));
  if(!valid){for(unsigned w=8;w<16;w++)assert(!memory.at(0x84500+w*4));return;}
  std::vector<uint64_t> samples;
  assert(!(clocks.size()%3));
  for(unsigned at=0;at<clocks.size();at+=3){
   assert(clocks[at].first==0xb80&&clocks[at+1].first==0xb00&&clocks[at+2].first==0xb80);
   if(clocks[at].second==clocks[at+2].second)samples.push_back((uint64_t(clocks[at].second)<<32)|clocks[at+1].second);
  }
  unsigned panels=type==8?2:16,blocks=groups*(inputs/(type==8?32:256)),stride=2+2*panels;
  assert(samples.size()==blocks*stride);uint64_t totals[3]={};
  for(unsigned b=0;b<blocks;b++){
   unsigned at=b*stride;totals[0]+=samples[at+1]-samples[at];
   for(unsigned p=0;p<panels;p++){totals[1]+=samples[at+2+2*p]-samples[at+1+2*p];totals[2]+=samples[at+3+2*p]-samples[at+2+2*p];}
  }
  for(unsigned p=0;p<3;p++)assert(((uint64_t(memory.at(0x84500+(9+2*p)*4))<<32)|memory.at(0x84500+(8+2*p)*4))==totals[p]&&totals[p]);
  assert(!memory.at(0x84538)&&!memory.at(0x8453c));
 }
};
int main(int argc,char** argv){
 assert(argc==3||(argc==4&&!strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}){
  if(argc==4&&(k!=5120||groups!=1||!work))continue;
  printf("PEER_PHASE_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DirectSrcBImage old(argv[1],type,k,groups);PeerPhaseImage next(argv[2],type,k,groups);assert(old.Boot()&&next.Boot());
  old.memory[0x74038]=next.memory[0x74038]=work;
  for(unsigned job=1;job<=2;job++){
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();old.PoisonTail();next.Poison();next.PoisonTail();next.clocks.clear();next.metricWrites=0;
   for(unsigned w=0;w<16;w++)next.memory[0x84500+w*4]=0x7fc00000;
   assert(old.Job(job)&&next.Job(job)&&next.CorrectRows());
   assert(old.requests==next.requests&&old.responses==next.responses&&old.matrixInstructions==next.matrixInstructions
    &&old.sumWrites==next.sumWrites&&old.sumReads==next.sumReads&&old.vectorOps==next.vectorOps&&old.drains==next.drains&&old.dotProbeReads==next.dotProbeReads);
   for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
   if(work)next.CheckMetrics(true,job);else{assert(next.clocks.empty()&&!next.metricWrites);for(unsigned w=0;w<16;w++)assert(next.memory.at(0x84500+w*4)==0x7fc00000);}
  }
  old.memory[0x74038]=next.memory[0x74038]=0;next.clocks.clear();next.metricWrites=0;assert(old.Job(3,2)&&next.Job(3,2)&&next.clocks.empty()&&!next.metricWrites);cases++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned fault=0;fault<3;fault++){
  PeerPhaseImage m(argv[2],type,5120,1);assert(m.Boot());m.memory[0x74038]=1;m.Poison();m.PoisonTail();
  m.rollover=fault==0;m.unstable=fault==1;m.backward=fault==2;
  assert(m.Job(1)&&m.CorrectRows()&&!m.pending&&m.pendingAccumulator.empty());m.CheckMetrics(fault==0,1);
 }
 for(unsigned fault:{1U,5U,40U,208U}){
  PeerPhaseImage m(argv[2],12,17408,3);assert(m.Boot());m.memory[0x74038]=1;m.faultAt=fault;
  assert(!m.Job(1)&&m.pending&&m.requests==fault&&!m.metricWrites&&!m.memory.at(0x74144));
  unsigned requests=m.requests;for(unsigned step=0;step<1000;step++)m.StepBlock();assert(m.pending&&m.requests==requests&&!m.metricWrites);
 }
 printf("PASS %sACTUAL peer timing:%ushapes/twochangedinputs+STOP,exactFP32/FP64/SAME64proofs/NoC/source/math/SFPU/drains;actualCSRstream exactlymatches telemetry;checked/STOP haveNOclock ormetricwrites;unstable/backward clocks invalidateDATAONLY and graphstillcompletes,realNoC faults remainpending/inert;NO native/timing qualification\n",argc==4?"PARTIAL ":"",cases);
}
