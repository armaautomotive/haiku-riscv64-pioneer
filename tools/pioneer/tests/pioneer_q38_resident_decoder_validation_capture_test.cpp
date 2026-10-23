/* SPDX-License-Identifier: MIT */
// ACTUAL existing-read diagnostic capture. NO new status/PC/device reads
// after observing a fault. This does not diagnose a physical card by retry.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
int main(int argc,char** argv){
 assert(argc==7);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);f.ProduceNative();auto& m=*f.c->q38EmbeddingModel;
 f.Attach(argv[1],argv[2]);f.transport->DecoderSources(argv[6],m.plan,m.bank,f.io);f.transport->ObserveDecoder(f.decoderResult);
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));for(unsigned role:{10U,11U,5U,6U})assert(RunQ38ProjectionExecution(*f.c,role,f.projection));
 for(unsigned fault:{7U,8U}){
  pid_t child=fork();assert(child>=0);
  if(!child){
   f.transport->DecoderFault(fault);assert(!RunQ38ResidentDecoderModel(*f.c,f.decoderResult));
   const auto& r=f.decoderResult;
   assert(r.phase==4 && r.worker==0 && r.observedWorker==0 && r.unsafe && !r.validationWords && !r.memory.restoredWords);
   if(fault==7)assert(r.word==40001 && r.observedStatus[2]==TT_Q38_CONV_BATCH_FAULT && !r.observedStatus[0] && !r.observedStatus[1]);
   else assert(r.word==40003 && r.observedStatus[2]==TT_Q38_CONV_BATCH_MARKER && r.observedStatus[0]==0x6400c && !r.observedStatus[1]);
   assert(!r.observedPc[0] && !r.observedPc[1]);f.UnknownNative();
   printf("PASS existing-read capture fault%u location%u status%08x/%08x/%08x,NO post-failure PC/status/END/restore/retry\n",
    fault,r.word,r.observedStatus[0],r.observedStatus[1],r.observedStatus[2]);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS2 actual DEBUG fault-marker versus terminal-count capture using original short-circuit reads only;ALL owners retained;SIMULATED NOT PHYSICAL DIAGNOSIS");
}
