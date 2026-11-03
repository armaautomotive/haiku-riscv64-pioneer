/* SPDX-License-Identifier: MIT */
#define PIONEER_DRAM_COHORT_IMAGE_TEST_COMPONENT
#include "pioneer_dram_cohort_read_sweep_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramCohortReadSweepOwner.h"
using Image=BlackholeDramCohortReadSweepImage;
struct CohortRig {
 std::unique_ptr<PioneerDramCohortReadSweepOwner> owner{new PioneerDramCohortReadSweepOwner};
 PioneerDramCohortReadSweepResult result;
 Fabric fabric;
 tt_dram_samples_result proof={};
 std::vector<uint32_t> backing;
 std::vector<uint32_t> original[8];
 uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
  0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
 uint32_t tlb[2][3]={{1,2,3},{4,5,6}},originalTlb[2][3]={{1,2,3},{4,5,6}};
 uint32_t reset[8]={0x47800,0x47800,0x47800,0x47800,0x47800,0x47800,0x47800,0x47800};
 unsigned stage[8]={},holds=0,clockBegins=0,clockEnds=0,allHeldReads=0,touches=0,pauses=0;
 unsigned l1Writes=0,resetWrites=0;
 bool running[8]={},quarantined=false,scope=true,clock=false,fastHost=false,freezeReady=false;
 bool snapshotCorrupt=false,restoreCorrupt=false,lateFifo=false,otherRunning=false,badCounter=false;
 unsigned corruptRole=3,corruptWord=17,failPhase=0,failKind=0;
 bool faultUsed=false;
 bool blindRoot=false,rootAdvanced=false,badProgress=false;
 uint64_t hostTime=1000000;
 enum Kind{Source=1,Pci,TlbRead,TlbWrite,NocRead,NocWrite,L1Read,L1Write,ResetRead,ResetWrite,
  Entry,Pc,Window,Time,ClockBegin,ClockEnd};
 CohortRig(const std::vector<uint8_t>& boot,const std::vector<uint8_t>& text,unsigned good,unsigned bad,unsigned depth=16)
  :fabric(boot,text,good,bad,depth),backing(PioneerDramCohortReadSweepOwner::Allocation,0xfeedabba){
  assert(Word(boot,0)==Image::Code(0) && text.size()==Image::TextWords*4);
  for(unsigned i=0;i<Image::TextWords;i++)assert(Word(text,i*4)==Image::Code(i+1));
  proof.version=1;proof.state=TT_DRAM_SAMPLE_ROUTES_OK;proof.routes_verified=7;
  proof.tlb_restored=proof.data_restored=1;memcpy(proof.source,source,sizeof(source));assert(BlackholeDramBuildRoutes(proof));
  // Captured P100A physical-bank order,NOT a route copied from a job or
  // completion. The standalone fixture deliberately uses another ordering.
  const uint32_t actual[7]={18|(12<<6),18|(15<<6),18|(18<<6),17|(12<<6),17|(15<<6),17|(18<<6),17|(21<<6)};
  for(unsigned bank=0;bank<7;bank++){
   fabric.bankRoutes[bank]=actual[bank];
   assert(actual[bank]==(proof.translated[bank][0]|(proof.translated[bank][1]<<6)));
  }
  for(unsigned r=0;r<8;r++){
   auto& m=*fabric.cores[r];m.config=source[0];m.memory.clear();
   original[r].resize(Image::SavedWords(r==0));
   for(unsigned i=0;i<original[r].size();i++){
    const uint32_t a=Image::Address(r==0,i),v=0x68a4bc19U^(r*0x80101001U)^(i*0x10203U);
    original[r][i]=v;WriteValue(r,a,v);
   }
  }
 }
 static unsigned X(unsigned r){return r?(C::Worker(r)&63):1;}
 bool Maps(unsigned s,uint32_t base,unsigned x,unsigned y,bool ro)const{
  for(unsigned i=0;i<3;i++)if(tlb[s][i]!=BlackholeDramFixedTlbWord(base,x,y,ro,i))return false;
  return true;
 }
 static bool Raw(unsigned a){return a>=C::Raw && a<C::Raw+0x40000;}
 uint32_t Value(unsigned r,unsigned a)const{return Raw(a)?fabric.cores[r]->raw[(a-C::Raw)/4]:fabric.cores[r]->memory.at(a);}
 void WriteValue(unsigned r,unsigned a,uint32_t v){if(Raw(a))fabric.cores[r]->raw[(a-C::Raw)/4]=v;else fabric.cores[r]->memory[a]=v;}
 bool Original()const{
  for(unsigned r=0;r<8;r++)for(unsigned i=0;i<original[r].size();i++)
   if(Value(r,Image::Address(r==0,i))!=original[r][i])return false;
  return true;
 }
 void Advance(unsigned count){
  for(unsigned i=0;i<count;i++)for(unsigned r=0;r<8;r++)if(running[r] && !fabric.cores[r]->Parked()){
   fabric.Tick();fabric.cores[r]->Step();
  }
 }
 bool Touch(unsigned kind){
  assert(!quarantined);touches++;
  if(result.phase==failPhase && kind==failKind && !faultUsed){faultUsed=true;return false;}
  Advance(1);return true;
 }
 unsigned CohortAllocationWords(){return unsigned(backing.size());}
 uint32_t* CohortSavedWords(){return backing.data();}
 uint32_t* CohortScratchWords(){return backing.data()+Image::CohortSavedWords;}
 bool CohortNativeScope(const void* o,const tt_dram_samples_result* d,const uint32_t* s,const uint32_t* scratch,unsigned sw,unsigned rw){
  return !quarantined && scope && o==owner.get() && d==&proof && s==backing.data()
   && scratch==backing.data()+Image::CohortSavedWords && sw==Image::CohortSavedWords && rw==Image::RawWords;
 }
 void CohortQuarantineMemoryOnly(){assert(!quarantined);quarantined=true;}
 bool CohortClockBegin(){if(!Touch(ClockBegin))return false;assert(!clock);clock=true;clockBegins++;return true;}
 bool CohortClockEnd(){
  if(!Touch(ClockEnd))return false;assert(clock && Original() && !memcmp(tlb,originalTlb,sizeof(tlb)));
  for(unsigned r=0;r<8;r++)assert(!running[r] && reset[r]==0x47800 && fabric.cores[r]->config==source[0]
   && fabric.cores[r]->queued.empty() && fabric.cores[r]->flying.empty());
  clock=false;clockEnds++;return true;
 }
 uint32_t SourceRead(unsigned i){assert(i<16);return Touch(Source)?source[i]:source[i]^1;}
 uint32_t HostPciRead(unsigned p,uint16_t a,uint8_t n){
  assert(p<2);if(!Touch(Pci))return UINT32_MAX;
  if(a==4 && n==2)return 0x402;
  if(a==0x34 && n==1)return 0x40;
  if(a==0x40 && n==2)return 0x10;
  if(a==0x42 && n==2)return 2;
  assert(a==0x4a && n==2);return 0;
 }
 uint32_t DramTlbRead(unsigned s,unsigned i){assert(s<2 && i<3);return Touch(TlbRead)?tlb[s][i]:tlb[s][i]^1;}
 void DramTlbWrite(unsigned s,unsigned i,uint32_t v){assert(s<2 && i<3);if(Touch(TlbWrite))tlb[s][i]=v;}
 uint32_t CohortHeldRead(unsigned x,unsigned y){
  assert(Maps(1,0xffa00000,x,y,true));if(!Touch(ResetRead))return 0;allHeldReads++;
  if(otherRunning && x==14 && y==11)return 0x46800;
  for(unsigned r=0;r<8;r++)if(x==X(r) && y==2)return reset[r];
  return 0x47800;
 }
 uint32_t CohortResetRead(unsigned r){assert(r<8 && Maps(1,0xffa00000,X(r),2,false));return Touch(ResetRead)?reset[r]:0;}
 uint32_t CohortEntryRead(unsigned r){assert(r<8 && Maps(1,0xffa00000,X(r),2,false));return Touch(Entry)?0:1;}
 uint32_t CohortNocRead(unsigned r,unsigned a){
  assert(r<8 && Maps(1,0xffa00000,X(r),2,false));if(!Touch(NocRead))return UINT32_MAX;
  auto& m=*fabric.cores[r];
  for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i))return i?source[i]:m.config;
  if(a==0x178)return 0;
  if(a==0x64)return m.config&0x10000?(lateFifo && result.phase==5 && r==0 && m.pc==Image::Park?0x1010100f:0x10101000|uint32_t(16-m.queued.size())):0;
  if(a>=0x200 && a<0x300)return m.counter[(a-0x200)/4]+unsigned(badCounter && result.phase==5 && r==0 && m.pc==Image::Park && a==0x208);
  return m.Load(0xffb20000+a);
 }
 void CohortNocWrite(unsigned r,unsigned a,uint32_t v){
  assert(r<8 && Maps(1,0xffa00000,X(r),2,false) && !running[r]);
  if(!Touch(NocWrite))return;
  auto& m=*fabric.cores[r];assert(m.queued.empty() && m.flying.empty());
  if(a==0x100){assert(v==source[0] || v==(source[0]|0x10000));m.config=v;}
  else{assert(a<40 && !(a&3));m.cmd[a/4]=v;}
 }
 bool CohortRead(unsigned r,unsigned a,uint32_t& v){
  assert(r<8 && Maps(0,0,X(r),2,false));if(!Touch(L1Read))return false;
  v=Value(r,a);
  if(badProgress && result.phase==5 && result.launched==255 && r==0 && a==C::Ready)v=0xbad;
  if(snapshotCorrupt && result.phase==2 && r==3 && a==Image::Address(false,3) && !faultUsed){v^=1;faultUsed=true;}
  return true;
 }
 bool CohortWrite(unsigned r,unsigned a,uint32_t v){
  assert(r<8 && Maps(0,0,X(r),2,false) && !running[r]);if(!Touch(L1Write))return false;
  l1Writes++;WriteValue(r,a,v);
  if(restoreCorrupt && result.phase==8 && r==3 && a==C::Raw)WriteValue(r,Image::Address(false,3),Value(r,Image::Address(false,3))^1);
  return true;
 }
 void CohortResetWrite(unsigned r,uint32_t v){
  assert(r<8 && Maps(1,0xffa00000,X(r),2,false));if(!Touch(ResetWrite))return;
  resetWrites++;
  if(v==0x87ffff){assert(stage[r]==0 && !running[r] && !fabric.cores[r]->steps);stage[r]=1;}
  else if(v==0x46800){
   assert(stage[r]==1 && reset[r]==0x87ffff);stage[r]=2;running[r]=!(freezeReady && r==3);
   if(!r)for(unsigned k=1;k<8;k++)assert(fabric.cores[k]->memory.at(C::Ready)==0x32435a67);
  }else{
   assert(v==0x47800 && stage[r]==2 && fabric.cores[r]->pc==Image::Park
    && fabric.cores[r]->queued.empty() && fabric.cores[r]->flying.empty());
   stage[r]=3;running[r]=false;holds++;
  }
  reset[r]=v;
 }
 uint32_t CohortPcRead(unsigned r){assert(r<8 && running[r]);return Touch(Pc)?fabric.cores[r]->pc:0;}
 bool CohortWindowRead(unsigned role,unsigned offset,uint32_t& v){
  assert(role>=1 && role<8 && !running[role] && result.held==255 && result.phase==7);
  if(!Touch(Window))return false;
  const unsigned bank=role-1;
  assert(Maps(0,0x04600000,proof.translated[bank][0],proof.translated[bank][1],true));
  v=Fabric::Pattern(role,0x04600000+offset)^unsigned(role==corruptRole && corruptWord!=UINT32_MAX
   && offset==0x200000-fabric.depth*16384+corruptWord*4);
  return true;
 }
 bool CohortHostTime(uint64_t& now){
  if(!Touch(Time))return false;
  if(blindRoot && result.phase==5 && result.launched==255 && !rootAdvanced){
   rootAdvanced=true;Advance(3000000);
  }
  now=hostTime+fabric.tick;return true;
 }
 void CohortPause(){
  assert(!quarantined);pauses++;
  // Accelerate ONLY the intended missing-ready/completion wait,not an
  // unrelated earlier reader's normal startup. Device ticks are already time.
  if(fastHost && ((freezeReady && (result.launched&8)) || result.launched==255))hostTime+=10000000;
  Advance(2048);
 }
 bool Run(){return owner->Execute(*this,proof,1,fabric.depth,0x32435a67,result);}
};
int main(int argc,char** argv){
 assert(argc==5);const auto boot=Read(argv[1]),text=Read(argv[2]);
 const unsigned good=unsigned(std::stoul(argv[3],nullptr,16)),bad=unsigned(std::stoul(argv[4],nullptr,16));
 unsigned valid=0,refused=0;
 {CohortRig r(boot,text,good,bad);
  assert(r.owner->Preflight(r,r.proof,r.result) && r.Original() && !r.result.started && !r.result.closed
   && !r.result.unsafe && !r.quarantined && !r.clockBegins && !r.clockEnds && !r.l1Writes && !r.resetWrites
   && !memcmp(r.tlb,r.originalTlb,sizeof(r.tlb)) && r.allHeldReads==240);
  for(const auto& c:r.fabric.cores)assert(!c->steps && !c->requests);
  const unsigned before=r.touches;PioneerDramCohortReadSweepResult again;
  assert(!r.owner->Preflight(r,r.proof,again) && r.touches==before);valid++;
 }
 for(unsigned fault=0;fault<4;fault++){
  CohortRig r(boot,text,good,bad);
  if(!fault)r.otherRunning=true;
  else {r.failPhase=0;r.failKind=fault==1?CohortRig::Entry:fault==2?CohortRig::TlbWrite:CohortRig::NocRead;}
  assert(!r.owner->Preflight(r,r.proof,r.result) && r.quarantined && r.result.unsafe
   && !r.l1Writes && !r.resetWrites && !r.clockBegins && !r.result.started);refused++;
 }
 for(unsigned depth:{1U,16U}){
  CohortRig r(boot,text,good,bad,depth);r.corruptWord=UINT32_MAX;
  assert(r.Run() && r.result.closed && !r.result.unsafe && !r.quarantined && r.Original());
  assert(r.result.launched==255 && r.result.observed==255 && r.result.held==255 && r.holds==8
   && r.result.savedWords==Image::CohortSavedWords && r.result.restoredWords==Image::CohortSavedWords
   && r.result.comparedWords==7*depth*4096 && r.clockBegins==1 && r.clockEnds==1 && !r.clock && r.allHeldReads==480);
  const auto& timing=r.result.timing;
  if(timing.state!=1 || timing.elapsedMinUs>timing.cycles || timing.cycles>timing.elapsedMaxUs)
   fprintf(stderr,"bracket depth%u state%u start%llu..%llu end%llu..%llu delta%llu..%llu cycles%llu\n",depth,
    timing.state,(unsigned long long)timing.startLowerUs,(unsigned long long)timing.startUpperUs,
    (unsigned long long)timing.endLowerUs,(unsigned long long)timing.endUpperUs,
    (unsigned long long)timing.elapsedMinUs,(unsigned long long)timing.elapsedMaxUs,(unsigned long long)timing.cycles);
  assert(timing.state==1 && timing.bytes==7*8388608 && timing.cycles
   && timing.elapsedMinUs<=timing.cycles && timing.cycles<=timing.elapsedMaxUs);
  const unsigned stopped=r.touches;PioneerDramCohortReadSweepResult again;
  assert(!r.owner->Execute(r,r.proof,1,depth,1,again) && r.touches==stopped);valid++;
 }
 {CohortRig r(boot,text,good,bad);r.corruptWord=UINT32_MAX;r.blindRoot=true;
  assert(r.Run() && r.rootAdvanced && r.result.closed && !r.quarantined && r.Original()
   && r.result.timing.state==2 && !r.result.timing.elapsedMinUs && !r.result.timing.elapsedMaxUs);valid++;
 }
 for(unsigned fault=0;fault<10;fault++){
  CohortRig r(boot,text,good,bad);r.corruptWord=UINT32_MAX;
  if(fault==0)r.otherRunning=true;
  if(fault==1)r.snapshotCorrupt=true;
  if(fault==2){r.freezeReady=true;r.fastHost=true;}
  if(fault==3){r.fabric.fault=Fabric::LostRead;r.fastHost=true;}
  if(fault==4){r.fabric.fault=Fabric::LostStartAck;r.fastHost=true;}
  if(fault==5)r.badCounter=true;
  if(fault==6)r.lateFifo=true;
  if(fault==7){r.corruptRole=7;r.corruptWord=17;}
  if(fault==8)r.restoreCorrupt=true;
  if(fault==9)r.badProgress=true;
  const bool accepted=r.Run();if(accepted)fprintf(stderr,"owner unexpected fault success %u\n",fault);
  assert(!accepted && r.quarantined && r.result.unsafe && !r.result.closed && !r.clockEnds);
  if(fault==2)assert(!r.holds && r.reset[0]==0x87ffff && r.pauses && !r.result.restoredWords
   && r.result.launched==14 && !r.fabric.cores[3]->steps);
  if(fault==3 || fault==4)assert(!r.holds && r.result.launched==255 && !r.result.restoredWords);
  if(fault==7)assert(r.result.held==255 && r.result.comparedWords==6*65536+17 && !r.result.restoredWords);
  const unsigned stopped=r.touches;PioneerDramCohortReadSweepResult again;
  assert(!r.owner->Execute(r,r.proof,1,16,1,again) && stopped==r.touches);refused++;
 }
 for(const auto& f:std::vector<std::pair<unsigned,unsigned>>{{1,CohortRig::ClockBegin},{2,CohortRig::L1Read},
  {3,CohortRig::L1Write},{4,CohortRig::ResetWrite},{5,CohortRig::Time},{6,CohortRig::ResetWrite},
  {7,CohortRig::Window},{8,CohortRig::L1Write},{8,CohortRig::L1Read},{9,CohortRig::TlbWrite},{9,CohortRig::ClockEnd}}){
  CohortRig r(boot,text,good,bad);r.corruptWord=UINT32_MAX;r.failPhase=f.first;r.failKind=f.second;
  assert(!r.Run() && r.faultUsed && r.quarantined && !r.result.closed);refused++;
 }
 printf("PASS_COHORT_OWNER_ACTUAL_RV32 valid%u refused%u all8virginEngines1 readyReadersBeforeRoot1 allSavedWords%u byteExactRestore1 fullLastBatches1 realPendingLossExternalDeadline1 noIOAfterQuarantine1;NATIVE_SCOPE_ARC_OS_ENDPOINTS_STILL_FIXTURES_NOT_HARDWARE_OR_BANDWIDTH_WORK\n",valid,refused,Image::CohortSavedWords);
}
