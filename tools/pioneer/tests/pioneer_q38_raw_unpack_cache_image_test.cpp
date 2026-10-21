/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
struct RawCacheImage:DirectSrcBImage {
 std::vector<uint8_t> fetchCode;bool gate=false,omitGate=false,omitPublish=false;
 unsigned publications=0,cacheGates=0;uint64_t producer[2]={UINT64_MAX,UINT64_MAX};
 unsigned missingProducer=0;struct CacheHazard{};
 RawCacheImage(const char* path,unsigned t,unsigned k,unsigned g,unsigned worker=69):DirectSrcBImage(path,t,k,g,worker){
  fetchCode=Read((std::string(path)+"/fetch.bin").c_str());assert(!fetchCode.empty()&&fetchCode.size()<=0x700&&!(fetchCode.size()&3));
  for(unsigned w=0;w<fetchCode.size()/4;w++)l1[Index(0x70000+w*4)]=Word(fetchCode,w*4);
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(fetchCode.size()))||DirectSrcBImage::CodeAddress(at);}
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x70000,unsigned(fetchCode.size())))return l1[Index(at)];return DirectSrcBImage::Load(at);
 }
 void Generation(){memset(decoded,0,sizeof(decoded));panelMoves=0;affineEligible=false;sourceProbeRows=0;}
 void Instruction(uint32_t op)override{
  const bool cached=Active()&&type==12;
  unsigned ordinal=matrixInstructions/MatrixPhasesPerPanel(),panel=ordinal%16;
  uint64_t block=ordinal/16;
  if(op==0xa2ff8020){assert(cached&&(panel&2)&&!gate);cacheGates++;if(!omitGate)gate=true;return;}
  if(op==0x43000007){
   if(!cached||!(panel&2)||!gate||rawPending||pending||!phaseQueue.empty()||!pendingAccumulator.empty()
    ||sources[unpackBank]||ownerA||producer[panel&1]!=block||missingProducer==1+(panel&1))throw CacheHazard{};
   gate=false;if(omitPublish)throw CacheHazard{};
   // Independent documented UNPACR_NOP/7 handoff;NO data read/write.
   sources[unpackBank]=true;unpackBank^=1;ownerA=sources[mathBank];
   Generation();publications++;drained=false;return;
  }
  if(cached&&(op>>24)==0x42&&!(op&0x800000)&&config[72]==14){
   assert(!(panel&2)&&!gate);unsigned row=((config[84]&65535)/16)-4;
   assert(row==16*(panel&1));
   unsigned expected=0x90000+256+(panel/4)*512+(panel&1)*256;
   assert((config[76]+1)*16==expected);
   if(!(panel&3))producer[0]=producer[1]=UINT64_MAX;
   producer[panel&1]=block;Generation();
  }
  DirectSrcBImage::Instruction(op);
 }
};
#ifndef PIONEER_Q38_RAW_CACHE_IMAGE_MODEL_ONLY
int main(int argc,char** argv){
 assert(argc==3||(argc==4&&!strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}){
  if(argc==4&&(k!=5120||groups!=1||!work))continue;
  printf("RAW_CACHE_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DirectSrcBImage old(argv[1],type,k,groups);RawCacheImage next(argv[2],type,k,groups);
  assert(old.Boot()&&next.Boot());old.memory[0x74038]=next.memory[0x74038]=work;
  for(unsigned job=1;job<=2;job++){
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();next.Poison();old.PoisonTail();next.PoisonTail();
   assert(old.Job(job)&&next.Job(job));
   assert(old.requests==next.requests&&old.responses==next.responses&&old.matrixInstructions==next.matrixInstructions
    &&old.sumWrites==next.sumWrites&&old.sumReads==next.sumReads&&old.vectorOps==next.vectorOps
    &&old.drains==next.drains&&!next.gate&&next.pendingAccumulator.empty());
   unsigned reused=work&&type==12?job*groups*(k/32):0;
   assert(next.publications==reused&&next.cacheGates==reused&&old.rawUnpacks==next.rawUnpacks+reused&&old.rawReads==next.rawReads+reused*256);
   for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2)&&next.Job(3,2));
 }
 for(unsigned failure=0;failure<4;failure++){
  RawCacheImage bad(argv[2],12,5120,1);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();
  bad.omitGate=failure==0;bad.omitPublish=failure==1;bad.missingProducer=failure>=2?failure-1:0;
  bool caught=false;try{(void)bad.Job(1);}catch(const RawCacheImage::CacheHazard&){caught=true;}
  assert(caught&&!bad.outputReads&&!bad.memory.at(0x74144));
 }
 for(unsigned fault:{1U,5U,40U,208U}){
  RawCacheImage bad(argv[2],12,17408,3);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.faultAt=fault;
  assert(!bad.Job(1)&&bad.pending&&bad.requests==fault&&bad.responses==fault-1&&!bad.memory.at(0x74144));
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
 }
 puts(argc==4?"PASS PARTIAL raw-cache Work pilots;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL raw-Dst cache ALL54 shapes/two changed inputs+STOP:original FP32+independent FP64/SAME all64proofs/matrix/SFPU/NoC/drains;ONLY Q4 raw unpack/read count halved,all SrcA handoffs retained;missing gate/publish/either producer refuses before output,NoC faults inert;NO native or speed qualification");
}
#endif
