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
 for(unsigned fault=7;fault<=14;fault++){
  pid_t child=fork();assert(child>=0);
  if(!child){
   f.transport->DecoderFault(fault);assert(!RunQ38ResidentDecoderModel(*f.c,f.decoderResult));
   const auto& r=f.decoderResult;
   unsigned worker=fault==12?119:0;
   assert(r.phase==4 && r.worker==worker && r.observedWorker==worker && r.unsafe && !r.validationWords && !r.memory.restoredWords);
   std::vector<uint32_t> expected;
   // Prior workers each prove BOTH receipts. The late-worker fault must
   // not inherit a successful earlier worker's status or PC observations.
   for(unsigned w=0;w<worker;w++)for(unsigned read:{0x102U,0x102U,0x100U,0x101U,0x200U,0x200U,0x102U,0x100U,0x101U,0x200U,0x200U})
    expected.push_back((w<<16)|read);
   auto append=[&](std::initializer_list<unsigned> reads){for(unsigned read:reads)expected.push_back((worker<<16)|read);};
   if(fault==7 || fault==12){
    assert(r.word==40001 && r.observedStatus[2]==TT_Q38_CONV_BATCH_FAULT && !r.observedStatus[0] && !r.observedStatus[1]);
    append({0x102});
   }else if(fault==9 || fault==11){
    assert(r.word==40004 && !r.observedStatus[0] && !r.observedStatus[1] && !r.observedStatus[2]);
    append({0x102,0x200});
   }else{
    assert(r.word==40003 && r.observedStatus[2]==TT_Q38_CONV_BATCH_MARKER);
    if(fault==13){assert(r.observedStatus[0]==0x64008 && !r.observedStatus[1]);append({0x102,0x102,0x100});}
    else{
     assert(r.observedStatus[0]==0x6400c && r.observedStatus[1]==(fault==8?0:96));
     append({0x102,0x102,0x100,0x101});
     if(fault==10)append({0x200});if(fault==14)append({0x200,0x200});
    }
   }
   assert(r.observedPc[0]==(fault==9?0x6000U:fault==11?0x6403cU:fault==10?0x64058U:fault==14?0x64038U:0U));
   assert(r.observedPc[1]==(fault==14?0x64058U:0U));
   assert(f.transport->DecoderCompletionReads()==expected);f.UnknownNative();
   assert(f.transport->DecoderCompletionReads()==expected);
   printf("PASS existing-read capture fault%u worker%u location%u status%08x/%08x/%08x pc%08x/%08x exact phase4 reads%zu,NO post-failure PC/status/END/restore/retry\n",
    fault,r.observedWorker,r.word,r.observedStatus[0],r.observedStatus[1],r.observedStatus[2],r.observedPc[0],r.observedPc[1],expected.size());_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS8 actual DEBUG fault-marker/count/phase/PC/late-worker capture with EXACT original short-circuit reads;ALL owners retained;SIMULATED NOT PHYSICAL DIAGNOSIS");
}
