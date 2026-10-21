/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
struct ReplayImage:DeferredAffineImage {
 bool omitAffineRecord=false;
 unsigned affineRecords=0,affineExpansions=0;
 unsigned injectedRawNegativeZeros=0;
 using DeferredAffineImage::DeferredAffineImage;
 void Instruction(uint32_t op)override {
  if(op>>24==4 && ((op>>14)&31)==12) {if(op&1)affineRecords++;else affineExpansions++;}
  if(omitAffineRecord && op>>24==4 && ((op>>14)&31)==12 && (op&1))dropReplayRecord=true;
  DeferredAffineImage::Instruction(op);
#ifdef PIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION
  if((op&~2U)==0x70040020)for(unsigned lane=0;lane<32;lane++)if(!(lr[0][lane]&0x7fffffff)) {
   lr[0][lane]=0x80000000;injectedRawNegativeZeros++;
  }
#endif
 }
};
#ifndef PIONEER_Q38_AFFINE_REPLAY_IMAGE_MODEL_ONLY
int main(int argc,char** argv) {
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned injectedTotal=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
 for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
  if(argc==4 && (
#ifndef PIONEER_Q38_TEST_DOT_ELISION_WITHOUT_REPLAY
   (type!=8 && type!=12) ||
#endif
   k!=5120 || groups!=1 || !work))continue;
  printf("AFFINE_REPLAY_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DeferredAffineImage old(argv[1],type,k,groups);ReplayImage next(argv[2],type,k,groups);
  assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
  for(unsigned job=1;job<=2;job++) {
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();next.Poison();unsigned before=old.steps,after=next.steps;
   assert(old.Job(job) && next.Job(job));
   assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
    && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.dotProbeReads==next.dotProbeReads
    && old.rawReads==next.rawReads && old.drains==next.drains);
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
   bool replayEligible=work && (type==8 || type==12);
#ifdef PIONEER_Q38_TEST_DOT_ELISION_WITHOUT_REPLAY
   replayEligible=false;
#endif
   assert(next.affineRecords==(replayEligible?job*groups:0));
   assert(next.affineExpansions==(replayEligible?job*groups*(2*(k/16)-1):0));
   if(work && groups==16 && job==2)printf("OFFLINE_AFFINE_REPLAY type%u K%u RV%u/%u vector_ops%u/%u drains%u/%u;SAME_ORIGINAL_WIRE_EXACT_OUTPUT_NOT_NATIVE_OR_TOKEN_RATE\n",
    type,k,old.steps-before,next.steps-after,old.vectorOps,next.vectorOps,old.drains,next.drains);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
  injectedTotal+=next.injectedRawNegativeZeros;
 }
#ifndef PIONEER_Q38_TEST_DOT_ELISION_WITHOUT_REPLAY
 for(unsigned type:{8U,12U}) {
  ReplayImage bad(argv[2],type,5120,1);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.omitAffineRecord=true;
  bool caught=false;try{(void)bad.Job(1);}catch(const ReplayImage::ReplayAccessFault&){caught=true;}
  assert(caught && !bad.outputReads && !bad.memory.at(0x74144));
 }
#endif
 for(unsigned fault:{1U,5U,40U,208U}) {
  ReplayImage bad(argv[2],12,17408,3);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.faultAt=fault;
  assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
 }
#ifdef PIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION
 if(argc==3)assert(injectedTotal);
 printf("RAW_SIGN_MAGNITUDE_NEGATIVE_ZERO_INJECTIONS %u;INDEPENDENT_SIGN_PRESERVING_CAST;FINAL_OUTPUT_EXACT_ORIGINAL\n",injectedTotal);
#else
 (void)injectedTotal;
#endif
 puts(argc==4?"PASS PARTIAL ONLY affine Work actual-image pilots+applicable record/NoC faults;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL affine Work:original FP32+FP64 exact,all formats/widths/groups/modes/two changed inputs,original wire/source/drains/matrix/STOP,checked unchanged,applicable record/NoC failure observers;NO_NATIVE_OR_SPEED_QUALIFICATION");
}
#endif
