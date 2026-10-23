/* SPDX-License-Identifier: MIT */
// ACTUAL native history/QPE1/QRN1/QPS1/QCB1 owner and fixed40page transport.
// Simulated OS/physical/learned endpoints,NOT original-model/card/speed proof.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
int main(int argc,char** argv){
 assert(argc==7 || (argc==8 && !strcmp(argv[7],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);f.ProduceNative();auto& m=*f.c->q38EmbeddingModel;
 f.Attach(argv[1],argv[2]);f.transport->DecoderSources(argv[6],m.plan,m.bank,f.io);f.transport->ObserveDecoder(f.decoderResult);
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));for(unsigned role:{10U,11U,5U,6U})assert(RunQ38ProjectionExecution(*f.c,role,f.projection));
 assert(RunQ38ResidentDecoderModel(*f.c,f.decoderResult) && f.decoderResult.stop.held==119 && f.decoderResult.held==120);
 auto values=std::make_unique<std::array<uint32,40960>>();
 for(unsigned kind=0;kind<5;kind++){
  uint32* output=values->data();if(kind==0)output=nullptr;if(kind==1)output=reinterpret_cast<uint32*>(uintptr_t(1));
  if(kind==2)output=reinterpret_cast<uint32*>(UINTPTR_MAX-3);if(kind==3)f.decoderResult.validationWords=1;if(kind==4)f.decoderResult.validationPages=1;
  unsigned n=f.io.touches,d=f.transport->Touches();
  assert(!m.decoder.Close(f.io,f.decoderResult,output) && !m.Unsafe() && f.io.touches==n && f.transport->Touches()==d);
  f.decoderResult.validationPages=f.decoderResult.validationWords=0;
 }
 if(argc==8){
  for(unsigned fault=1;fault<=4;fault++)for(unsigned page:{1U,10U,11U,40U}){
   pid_t child=fork();assert(child>=0);
   if(!child){
    f.transport->ValidationFault(fault,page);
    assert(!CloseQ38ResidentDecoderModel(*f.c,f.decoderResult,values->data()) && f.decoderResult.unsafe && !f.decoderResult.closed
     && !f.decoderResult.memory.restoredWords && f.transport->ValidationReads()==page);
    f.UnknownNative();printf("PASS actual retained-convolution DEBUG fault%u page%u ALL retained/no following device/END/restore IO\n",fault,page);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS16 actual fixed40page retained-convolution DEBUG stuck-packet/stale-last-word/counter/source faults first/boundary/last;SIMULATED NOT CARD");return 0;
 }
 assert(CloseQ38ResidentDecoderModel(*f.c,f.decoderResult,values->data()) && m.Closed()
  && f.decoderResult.validationPages==40 && f.decoderResult.validationWords==40960 && f.transport->ValidationReads()==40
  && f.decoderResult.memory.restoredWords==8134072 && f.decoderResult.memory.verifiedOriginalWords==8134072);
 for(unsigned i=0;i<40960;i++)assert((*values)[i]==f.io.wholeWorkspace[(i<10240?TT_Q38_CONV_OUTPUT:TT_Q38_CONV_NEXT)/4+(i<10240?i:i-10240)]);
 assert(!f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c) && !f.c->Unsafe());
 puts("PASS actual same native owner40 fresh-poisoned fixed pages/ALL40960 original-device words/ALL120 physically held BEFORE read/BEFORE full8134072word restore/actual END;5 inert pointer/result refusals;SIMULATED NOT ORIGINAL_GGUF_PHYSICAL_CARD_OR_SPEED");
}
