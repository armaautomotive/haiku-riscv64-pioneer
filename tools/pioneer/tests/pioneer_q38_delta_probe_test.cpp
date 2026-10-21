/* SPDX-License-Identifier: MIT */
// REAL relocated RV32/SFPU image through the REAL kernel lease templates.
// Fault-injected MMIO only; this is NOT native timing or driver authority.
#define PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
#ifdef PIONEER_Q38_LOCAL_DELTA_GGML
#define PIONEER_Q38_DELTA_VECTOR_POST_SCALE
#endif
#define PIONEER_Q38_DELTA_VECTOR_IMAGE_MACHINE_ONLY
#include "pioneer_q38_delta_vector_image_test.cpp"
#define PIONEER_Q38_PANEL_POLICY_MOCK_ONLY
#define B_DEVICE_OP_CODES_END 1000
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#include "pioneer_q38_panel_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DeltaProbe.h"
#include "../PioneerQ38DeltaProbeDebug.h"
struct DeltaIO:PanelIO {
 const char* image;
 unsigned fault=0,reads=0,writes=0,resetWrites=0,exportReads=0,pcMode=0,pcReads=0;
 bool restoring=false;
 explicit DeltaIO(const char* path):image(path) {}
 uint32_t Q38PanelRead(uint32_t a) {
  reads++; uint32_t v=PanelIO::Q38PanelRead(a);
  if (a>=0xa0000 && a<0xa8000 && coreReset==0x46800) exportReads++;
  if (restoring && fault==20 && a==TT_Q38_PANEL_VIEW_BASE) v^=1;
  return v;
 }
 void Q38PanelWrite(uint32_t a,uint32_t v) { writes++; PanelIO::Q38PanelWrite(a,v); }
 uint32_t RuntimeCodeRead(unsigned w) { reads++; return PanelIO::RuntimeCodeRead(w); }
 void RuntimeCodeWrite(unsigned w,uint32_t v) { writes++; PanelIO::RuntimeCodeWrite(w,v); }
 uint32_t RuntimeControlRead(unsigned w) { reads++; return PanelIO::RuntimeControlRead(w); }
 uint32_t ResetRead() { reads++; return PanelIO::ResetRead(); }
 uint32_t RuntimeStatusRead(unsigned w) { reads++; return PanelIO::RuntimeStatusRead(w); }
 void RuntimeStatusWrite(unsigned w,uint32_t v) { writes++; PanelIO::RuntimeStatusWrite(w,v); }
 uint32_t RuntimePcRead() {
  reads++; pcReads++;
  if (fault==4 || (fault==17 && exportReads)) return TT_Q38_DELTA_PARK+8;
  if (fault==22) return 0x72060;
  if (fault==23) return 0x7207c;
  return TT_Q38_DELTA_PARK+((pcMode==1 || (pcMode==2 && pcReads%2))?4:0);
 }
 uint32_t ResidentNocRead(uint32_t a) { reads++; return PanelIO::ResidentNocRead(a); }
 void RuntimeResetWrite(uint32_t value) {
  resetWrites++; resets.push_back(value);
  if (value==0x87ffff && ((launches && fault==18) || (!launches && fault==19))) return;
  if (value==0x47800 && fault==21) return;
  if (value==0x46800 && fault==1) { launches++; return; }
  coreReset=value;
  if (value!=0x46800) return;
  launches++;
  if (fault==2) return;
  if (fault==3) { destination[2034]=0x46564451; return; }
  DeltaVectorImage m(image,1);
  auto cp=[&](uint32_t at,unsigned count) { for (unsigned w=0;w<count;w++) m.memory[at+w*4]=PanelIO::Q38PanelRead(at+w*4); };
  cp(0x74000,4); cp(0x90000,8192); cp(0x8e000,322);
  assert(m.RunVector()); m.Verify();
  for (unsigned w=0;w<8192;w++) view[(0xa0000-TT_Q38_PANEL_VIEW_BASE)/4+w]=m.memory.at(0xa0000+w*4);
  for (unsigned w=0;w<64;w++) view[(0x8e800-TT_Q38_PANEL_VIEW_BASE)/4+w]=m.memory.at(0x8e800+w*4);
  destination[2032]=TT_Q38_DELTA_DONE; destination[2033]=64; destination[2034]=0x31564451;
  if (fault==5) lateCounter=true;
  if (fault==6) config^=1;
  if (fault==7) view[(TT_Q38_DELTA_CODE-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
  if (fault==8) view[(TT_Q38_DELTA_CODE+TT_Q38_DELTA_CODE_BYTES-4-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
  if (fault==9) destination[2032]++;
  if (fault==10) destination[2033]++;
  if (fault==11) destination[2034]^=1;
  if (fault==12) view[(0x90000-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
  if (fault==13) view[(0x97ffc-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
  if (fault==14) view[(0x8e500-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
  if (fault==15) view[(0xa7ffc-TT_Q38_PANEL_VIEW_BASE)/4]=0x7fc00000;
  if (fault==16) view[(0x8e8fc-TT_Q38_PANEL_VIEW_BASE)/4]=0x7f800000;
 }
};
struct DeltaFixture {
 DeltaIO io; PanelOwners own; PioneerQ38DeltaProbeLease lease;
 std::unique_ptr<tt_q38_delta_probe> job{new tt_q38_delta_probe{}};
 explicit DeltaFixture(const char* path,unsigned trial=1):io(path),own(io) {
  PioneerQ38DeltaDebugFixture(*job,trial);
 }
 bool Prepare() { return PioneerQ38DeltaProbePrepare(io,own.dram,own.noc,own.store,own.banks,lease); }
 bool RunDelta() { return PioneerQ38DeltaProbeRun(io,own.dram,own.noc,own.store,own.banks,lease,*job); }
 bool CloseDelta(bool clean=true) { return PioneerQ38DeltaProbeClose(io,own.dram,own.noc,own.store,own.banks,lease,clean); }
 bool Accept() { tt_q38_delta_accept a={TT_Q38_DELTA_PROBE_VERSION,lease.nonce,{0,0}}; return PioneerQ38DeltaProbeAccept(lease,a); }
};
#ifndef PIONEER_Q38_DELTA_PROBE_MOCK_ONLY
int main(int argc,char** argv)
{
 assert(argc==2);
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
 DeltaVectorImage image(argv[1],1);
 assert(image.code.size()==TT_Q38_DELTA_CODE_BYTES && image.l1[0]==TT_Q38_DELTA_BOOT);
 for (unsigned w=0;w<image.code.size()/4;w++) assert(image.l1[1+w]==PioneerQ38DeltaProbeWord(w));
 for (unsigned mode=0;mode<3;mode++) for (unsigned trial=0;trial<4;trial++) {
  DeltaFixture f(argv[1],trial); auto original=f.io.view; uint32_t entry=f.io.entry;
  f.io.pcMode=mode;
  assert(f.Prepare() && f.RunDelta() && f.lease.awaiting && f.lease.nonce==1 && !PioneerQ38DeltaDebugErrors(*f.job));
  unsigned reads=f.io.reads,writes=f.io.writes,resets=f.io.resetWrites;
  assert(!f.CloseDelta() && !f.RunDelta() && reads==f.io.reads && writes==f.io.writes && resets==f.io.resetWrites);
  for (unsigned kind=0;kind<5;kind++) {
   tt_q38_delta_accept a={TT_Q38_DELTA_PROBE_VERSION,1,{0,0}};
   if (!kind) a.version=1; if (kind==1) a.nonce=0; if (kind==2) a.nonce=2; if (kind==3) a.reserved[0]=1; if (kind==4) a.reserved[1]=1;
   assert(!PioneerQ38DeltaProbeAccept(f.lease,a) && f.lease.awaiting);
  }
  assert(f.Accept() && !f.Accept() && f.Prepare() && f.RunDelta() && f.lease.nonce==2 && !PioneerQ38DeltaDebugErrors(*f.job) && f.Accept());
  assert(f.CloseDelta() && f.io.view==original && f.io.entry==entry && !f.io.requests);
  Finish(f.io,f.own);
 }
 for (unsigned fault=1;fault<=23;fault++) {
  if (fault==20) continue;
  DeltaFixture f(argv[1]); assert(f.Prepare()); f.io.fault=fault;
  assert(!f.RunDelta() && f.lease.unsafe && f.own.store.recovery_required && f.job->result.recovery_required);
  unsigned reads=f.io.reads,writes=f.io.writes,resets=f.io.resetWrites;
  assert(!f.CloseDelta() && !f.Prepare() && !f.RunDelta() && !f.Accept());
  assert(reads==f.io.reads && writes==f.io.writes && resets==f.io.resetWrites);
  for (auto v:f.job->next) assert(!v); for (auto v:f.job->output) assert(!v);
 }
 {
  DeltaFixture f(argv[1]); assert(f.Prepare());
  f.io.view[(0x7203c-TT_Q38_PANEL_VIEW_BASE)/4]=0x001802b7; // old image fault entry is NEVER a safe terminal
  assert(!f.RunDelta() && f.lease.unsafe && !f.io.resetWrites && !f.io.exportReads);
 }
 {
  DeltaFixture f(argv[1]); assert(f.Prepare() && f.RunDelta() && f.Accept());
  f.io.fault=20; f.io.restoring=true; assert(!f.CloseDelta());
  unsigned reads=f.io.reads,writes=f.io.writes,resets=f.io.resetWrites;
  assert(!f.CloseDelta() && reads==f.io.reads && writes==f.io.writes && resets==f.io.resetWrites);
 }
 for (unsigned bad=0;bad<10;bad++) {
  DeltaFixture f(argv[1]);
  if (!bad) f.job->version=1; if (bad==1) f.job->reserved[2]=1;
  if (bad==2) f.job->prior[8191]=1; if (bad==3) f.job->query[127]=0x7f800000;
  if (bad==4) f.job->key[0]=0x80000001; if (bad==5) f.job->value[63]=0x7fc00000;
  if (bad==6) f.job->beta=0x80000000; if (bad==7) f.job->decay=0x3f800001;
  if (bad==8) f.job->decay=1; if (bad==9) f.job->beta=0x7f800000;
  assert(!PioneerQ38DeltaProbeValid(*f.job) && !f.RunDelta() && !f.io.reads && !f.io.writes && !f.io.resetWrites);
  Finish(f.io,f.own);
 }
 {
  DeltaFixture f(argv[1]); f.job->version=TT_Q38_DELTA_PROBE_VERSION==3?2:3;
  assert(!PioneerQ38DeltaProbeValid(*f.job) && !f.RunDelta() && !f.io.reads && !f.io.writes && !f.io.resetWrites);
  Finish(f.io,f.own); // opposite scaling ABI MUST refuse BEFORE IO
 }
 puts("PASS OFFLINE real LOCAL vector lease+actual image:ALL8256 words,4 domains x3 frontend modes,safe branch execution,old fault-entry image refused,23 fault points,invalid data inert,NO I/O after uncertainty/mismatch/unaccepted result;NOT native/resident/performance");
}
#endif
