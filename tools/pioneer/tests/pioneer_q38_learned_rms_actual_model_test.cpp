/* SPDX-License-Identifier: MIT */
// Actual DEBUG readback controller over ORIGINAL GGUF/emitted machines.
// Independent CPU oracle is compiled separately,never into this native TU.
#define PIONEER_Q38_RMS_VALIDATION_FIXTURE_ONLY
#include "pioneer_q38_learned_rms_native_validation_test.cpp"
#include "pioneer_q38_learned_rms_actual_source.h"
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("DEBUG_ORIGINAL_GGUF_FULL_SHA_BEFORE_ANY_SIMULATED_NATIVE_CONTEXT;NO_DEVICE_ACCESS_OR_SPEED_CLAIM");
 auto source=std::make_unique<RmsActualSource>();assert(source->Open(argv[4]) && source->Ready());
 puts("FULL_SAME_FILE_c600de03_SHA_AND_ALL851_TENSORS_PASS");
 for(unsigned token:{0U,15U,248319U}){
  assert(source->Token(token)); // BOTH independent references BEFORE even simulated native owner
  RmsValidationFixture f(argv[1],argv[2],argv[3],token,token?6:0);
  f.io.originalEmbeddingWeights=source->embedding.data();f.io.originalRmsWeights=source->norm.data();
  assert(f.Bind() && f.Initialize() && f.Run() && f.Normalize() && source->Ready());
  for(unsigned i=0;i<5120;i++)assert(f.normalized[i]==source->normalizedReference[i] && f.residual[i]==source->rawReference[i]);
  assert(f.norm.validationPages==10 && f.norm.validationWords==10240 && f.io.validationReads==10
   && f.norm.restoredWords==73737 && f.norm.verifiedWords==147474 && EndFactory(*f.c) && source->Ready());
  printf("PASS ORIGINAL GGUF token%u/workspace%u actual native zero/QPE1/QRN1/10fixed poisoned readback pages/10240original-reference words BIT-EXACT/full ROOT restore/END;SIMULATED_HARDWARE_NOT_CARD_OR_SPEED\n",token,token?6:0);
 }
}
