/* SPDX-License-Identifier: MIT */
// ACTUAL FD-close release function on the SAME native model/context after
// successful DEBUG computation. Closing without explicit END MUST retain,
// even when the userspace numerical mismatch is invisible to the driver.
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CONTROL_FIXTURE_ONLY
#include "pioneer_q38_resident_decoder_validation_control_test.cpp"
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH 1
#define TT_PIONEER_Q38_WORK_MODE 1
static unsigned forbiddenCleanup=0;
static void EndResidentSession(PioneerProbeCookie*,tt_resident_session_result&,bool){
 forbiddenCleanup++;assert(!"implicit FD-close device cleanup is FORBIDDEN for a retained decoder model");
}
#include "decoder_validation_cookie_release.inc"
int main(int argc,char** argv){
 assert(argc==7);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 DecoderControlFixture f(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6]);auto reply=Request();
 assert(ProbeControl(&f.cookie,TT_PCI_PROBE_Q38_DECODER_VALIDATE,reply.get(),sizeof(*reply))==B_OK
  && reply->metadata.closed && !reply->metadata.recovery_required && f.c->q38EmbeddingModel->Closed() && !f.c->Unsafe());
 for(unsigned priorCold=0;priorCold<2;priorCold++){
  pid_t child=fork();assert(child>=0);
  if(!child){
   if(priorCold){tt_resident_session_result result={};RetainQ38DispatchOwner(*f.c,result);}
   unsigned touches=f.io.touches,device=f.transport->Touches(),acquired=locks,released=releases;
   auto* exact=f.cookie.context;
   assert(ReleaseResidentCookie(&f.cookie)==B_OK && f.cookie.context==exact && f.cookie.magic==0x50313030 && sRuntimeOwner==&f.cookie
    && f.c->dispatchOwnerRetained && sColdRecoveryRequired && !forbiddenCleanup && !locked
    && locks==acquired+1 && releases==released+1 && f.io.touches==touches && f.transport->Touches()==device);
   f.UnknownNative();printf("PASS actual FD CLOSE ONLY priorCold%u retained exact cookie/context/model/snapshots/ALLhost+bulk+clock leases;ZERO endpoint/status/END/restore/free IO\n",priorCold);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 // Parent stayed clean;ONLY its explicit actual END may release normally.
 assert(!sColdRecoveryRequired && !f.c->dispatchOwnerRetained && !f.c->Unsafe() && !forbiddenCleanup && EndFactory(*f.c));
 puts("PASS2 actual native decoder FD CLOSE retention clean-metadata and already-cold paths/global lock released/NO implicit cleanup/normal explicit-END sibling preserved;SIMULATED_NOT_PHYSICAL_CARD");
}
