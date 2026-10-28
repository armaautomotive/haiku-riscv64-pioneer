/* SPDX-License-Identifier: MIT */
// FRESH execution of the known failing image from reset. No state transplant.
// Optimized host diagnostics are not sanitizer/full-model qualification.
#define PIONEER_Q38_LAYER_CHAIN64_MACHINE_ONLY
#include "pioneer_q38_layer_chain64_instruction_test.cpp"
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--prepare-only")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<ChainInstruction64Machine>(argv[1],argv[2]);
 if(argc==4){
  assert(!m->steps && !m->packets && !m->peerWrites && !m->completed);m->VerifyRawZeros();
  for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));
  puts("DIAGNOSTIC RAW setup checked/SAME119 virgin/ZERO ROOT instructions;NOT model pass or fault reproduction");return 0;
 }
 puts("START FRESH QCH1 fault diagnostic/actual pinned instructions/SAME119 from reset/ONE fabric/RAW only/optimized host WITHOUT sanitizers;NOT a qualification pass or speed benchmark");
 if(m->Run()){
  puts("DIAGNOSTIC ended without reproducing the prior fault;NOT automatically a qualification pass");return 2;
 }
 assert(m->pc==0x60000);
 unsigned packets=m->packets;for(unsigned i=0;i<2048;i++)m->ObservedStep();assert(m->pc==0x60000 && m->packets==packets);
 puts("DIAGNOSTIC actual emulated fault captured/inert2048/no recovery or completed state supplied;exit0 means CAPTURE ONLY,NOT model/driver/native/physical/Work/TPS pass");
}
