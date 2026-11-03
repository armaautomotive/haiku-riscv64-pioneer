/* SPDX-License-Identifier: MIT */
// Actual old/new native IO methods with counted KernelExport/PCI endpoints.
// No native prerequisite/ARC/engine/card execution or measured elapsed time.
#define COHORT_SCOPE_COUNTER_TEST 1
#ifndef COHORT_SCOPE_REFERENCE_TEST
#define COHORT_DIRECT_ENDPOINT_TEST 1
#endif
#define main CohortPublicRegressionMain
#include "pioneer_dram_cohort_public_test.cpp"
#undef main
struct DirectFixture {
 PioneerProbeCookie fd;
 PioneerDramCohortNativeContext* c;
 DirectFixture(){
  Reset();c=new PioneerDramCohortNativeContext(&fd,thread_info{},16);
  sDramSweepNativeContext=c;sDramCohortNativeContext=c;c->reserved=true;
  c->backing=(uint32*)calloc(c->AllocationWords,4);c->exactBacking=c->backing;
  c->profile.root_bus=0;c->profile.bar0_host_start=UINT64_C(0x4900000000);
  assert(c->io.Map() && views.size()==17);c->ready=c->cohortReady=true;assert(c->CohortScope());
 }
 void Select(unsigned r){
  for(unsigned i=0;i<3;i++){
   c->io.DramTlbWrite(0,i,BlackholeDramFixedTlbWord(0,PioneerDramCohortX(r),2,false,i));
   c->io.DramTlbWrite(1,i,BlackholeDramFixedTlbWord(0xffa00000,PioneerDramCohortX(r),2,false,i));
  }
 }
 static void Count(){areaQueries=wordReads=wordWrites=0;}
 ~DirectFixture(){
  // Fixture-only teardown,NEVER production disposal of retained resources.
  for(const auto& v:views)free(v.second.address);views.clear();
  delete c;sDramSweepNativeContext=NULL;sDramCohortNativeContext=NULL;sColdRecoveryRequired=false;
 }
};
int main(){
 assert(CohortPublicRegressionMain()==0);
 unsigned good=0,bad=0;
#ifdef COHORT_SCOPE_REFERENCE_TEST
 constexpr unsigned duplicate=34;
#else
 constexpr unsigned duplicate=0;
#endif
 for(unsigned r=0;r<8;r++){
  DirectFixture f;f.Select(r);auto& io=f.c->cohortIO;
  f.c->io.sweepMemory[0x74000/4]=0x13579bdf;uint32 v=0;
  f.Count();assert(io.CohortRead(r,0x74000,v) && v==0x13579bdf && areaQueries==17+6*duplicate
   && wordReads==7 && !wordWrites);good++;
  f.Count();assert(io.Selected(r,false) && areaQueries==17+3*duplicate && wordReads==3 && !wordWrites);good++;
  f.c->resetStages[r]=2;f.Count();assert(!io.CohortWrite(r,0x74000,1)
   && areaQueries==17+6*duplicate && wordReads==6 && !wordWrites);bad++;
 }
 for(unsigned r=0;r<8;r++)for(unsigned word=0;word<6;word++){
  DirectFixture f;f.Select(r);auto& io=f.c->cohortIO;
  auto* table=f.c->io.tlb-3;table[word]^=1;uint32 v=0xfeedabba;f.Count();
  const unsigned prefix=word<3?2*word+1:2*(word-3)+2;
  assert(!io.CohortRead(r,0x74000,v) && v==0xfeedabba && wordReads==prefix && !wordWrites
   && areaQueries==17+prefix*duplicate);bad++;
  io.CohortQuarantineMemoryOnly();const unsigned stopped=ioCalls;
  assert(!io.CohortRead(r,0x74000,v) && ioCalls==stopped);bad++;
 }
 for(unsigned area=0;area<17;area++)for(unsigned fault=0;fault<4;fault++){
  DirectFixture f;f.Select(0);auto it=views.begin();std::advance(it,area);const auto original=it->second;
  if(fault==0)it->second.address=(char*)it->second.address+4;
  if(fault==1)it->second.size+=4096;
  if(fault==2)it->second.protection|=B_WRITE_AREA;
  if(fault==3)it->second.area++;
  uint32 v=0xfeedabba;f.Count();
  assert(!f.c->cohortIO.CohortRead(0,0x74000,v) && f.c->quarantined && v==0xfeedabba
   && !wordReads && !wordWrites && areaQueries<=17);bad++;
  const unsigned stopped=ioCalls;assert(!f.c->cohortIO.Selected(0) && ioCalls==stopped);bad++;
  it->second=original; // fixture allocator owns original address
 }
 for(unsigned fault=0;fault<10;fault++){
  DirectFixture f;f.Select(0);
  if(fault==0)f.fd.magic=0;
  if(fault==1)currentThread++;
  if(fault==2)currentTeam++;
  if(fault==3)sReloadGeneration++;
  if(fault==4)f.c->exactBacking++;
  if(fault==5)f.c->cohortReady=false;
  if(fault==6)f.c->io.tlb++;
  if(fault==7)f.c->io.sweepMemory++;
  if(fault==8)sDramCohortNativeContext=NULL;
  if(fault==9)f.c->clockCall=true;
  f.Count();uint32 v=0xfeedabba;
  assert(!f.c->cohortIO.CohortRead(0,0x74000,v) && f.c->quarantined && !wordReads && !wordWrites);bad++;
 }
 {DirectFixture f;f.Select(0);uint32 v=1;f.Count();
  assert(!f.c->cohortIO.CohortRead(8,0x74000,v) && !areaQueries && !wordReads && !wordWrites);bad++;
  assert(!f.c->cohortIO.CohortRead(0,0x74080,v) && !areaQueries && !wordReads && !wordWrites);bad++;
 }
 printf("PASS_COHORT_SELECTED_ENDPOINTS valid%u refused%u viewsPerDataAccess%u tlbReads6 payloadReads1 all17DescriptorFaults1 "
  "all8rolesAll6TlbFaults1 noIOAfterRetention1;OPERATION_COUNTS_NOT_HARDWARE_TIMING_MODEL_SPEEDUP_OR_WORK\n",good,bad,17+6*duplicate);
}
