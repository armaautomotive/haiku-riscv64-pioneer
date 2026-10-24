/* SPDX-License-Identifier: MIT */
// Emitted QOP1 ROOT instructions only. These peers/proofs/products are
// deliberately SYNTHETIC transport endpoints,not the arithmetic suite.
#define PIONEER_Q38_GDN_OUTPUT_GRAPH_MACHINE_ONLY
#include "pioneer_q38_gdn_output_graph_image_test.cpp"
struct GdnGraphTransport: GdnGraphMachine {
 unsigned shadowMutatePacket=0,shadowMutateWord=0;
 GdnGraphTransport(const char* root,const char* peer):GdnGraphMachine(root,peer){
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)node[w].src[i]=fixture.sources[w][i];
 }
 uint32_t PhysicalRegister(unsigned w,unsigned a)override{return ProjectionParentMachine::PhysicalRegister(w,a);}
 uint32_t RowOutput(unsigned w,unsigned row)const override{return ProjectionParentMachine::RowOutput(w,row);}
 void PeerPacket()override{ProjectionParentMachine::PeerPacket();}
 void Advance(unsigned w)override{
  auto& n=node[w];if(n.op!=6){ProjectionParentMachine::Advance(w);return;}
  auto frame=n.frame;auto job=n.job;
  n.frame[0]=TT_Q38_PROJECTION_INPUT_MARKER;n.frame[10]-=TT_Q38_GDN_GATED_OUTPUT;n.frame[44]=n.frame[45]=0;
  n.job[0]=TT_Q38_PROJECTION_INPUT_MARKER;n.job[13]=5;n.op=5;
  ProjectionParentMachine::Advance(w);n.frame=frame;n.job=job;n.op=6;
  if(n.sequence==n.requested && n.verified){
   unsigned at=n.frame[13]/4;gddr.at(at+8)=TT_Q38_GDN_OUTPUT_INPUT_MARKER;
   gddr.at(at+29)=TT_Q38_GDN_RENDEZVOUS_MARKER;gddr.at(at+30)=n.frame[45];
   n.proof[48]=6;n.proof[62]=TT_Q38_GDN_OUTPUT_INPUT_MARKER;
   assert(PioneerQ38GdnOutputInputLine(gddr.data()+at,n.frame.data()));
  }
 }
 void Packet()override{
  GdnGraphMachine::Packet();
  if(packets==shadowMutatePacket)memory[TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW+shadowMutateWord*4]^=1;
 }
 void VerifyTransport(){
  assert(!pending && !unknown && producerJobs==24 && matrixJobs==119 && outputWrites);
  uint32_t s[64],before[8],after[8];
  for(unsigned i=0;i<64;i++)s[i]=memory.at(TT_Q38_PROJECTION_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++){before[i]=s[16+i];after[i]=counts[i==6?2:i];}
  assert(PioneerQ38GdnOutputGraphReceipt(s,manifest.data(),before,after));
  for(unsigned row=0;row<5120;row++)assert(outputCover[row]==1 && gddr.at(TT_Q38_GDN_OUTPUT_RESULT/4+row)==Bits(float(row+1)));
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 unsigned faults=0,packets=0;
 {auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);assert(m->RunGdn());m->VerifyTransport();packets=m->packets;
  printf("PASS QOP1 full ROOT transport baseline packets%u;SYNTHETIC PEER ARITHMETIC/NOT_NATIVE\n",packets);}
 for(unsigned word=0;word<64;word++){
  auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);m->badNode=1;m->badProofWord=word;
  assert(!m->RunGdn() && !m->lineReads && !m->outputWrites);faults++;
 }
 puts("PASS QOP1 ALL64 op6 producer-proof faults BEFORE gather");
 for(unsigned word:{0U,15U,16U,24U*16-1,119U*16-1})for(bool shadow:{false,true}){
  auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);
  if(shadow){m->shadowMutatePacket=1;m->shadowMutateWord=word;}else{m->sourceMutatePacket=1;m->sourceMutateWord=word;}
  assert(!m->RunGdn() && !m->producerJobs && !m->matrixJobs && !m->outputWrites);faults++;
 }
 for(unsigned word:{0U,1U,2U,3U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U,40U,41U,42U,43U,44U,45U,46U,48U,63U}){
  auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);m->memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+word*4]^=1;
  assert(!m->RunGdn() && !m->packets && !m->sourceShadowWrites && !m->outputWrites);faults++;
 }
 puts("PASS QOP1 source/shadow edge faults and strict wrong-frame pre-entry refusals");
 for(unsigned kind=0;kind<3;kind++){
  auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);
  if(!kind)m->badPcNode=1;if(kind==1)m->badSourceNode=1;if(kind==2)m->badCounterNode=1;
  assert(!m->RunGdn() && !m->lineReads && !m->outputWrites);faults++;
 }
 for(unsigned word:{0U,31U,63U}){
  auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);m->badNode=119;m->badProofWord=word;
  assert(!m->RunGdn() && m->producerJobs==24 && m->matrixJobs==119 && !m->outputWrites);faults++;
 }
 for(unsigned packet:{1U,packets}){
  auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);m->dropPacket=packet;
  assert(!m->RunGdn() && m->packets==packet);faults++;
 }
 // A late manifest change cannot turn completed output bytes into a passed
 // receipt. No packet/CSR/scratch writes after the resulting first fault.
 {auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);m->mutateAtPacket=packets;m->mutateWord=32;
  assert(!m->RunGdn() && m->outputWrites && m->packets==packets);faults++;}
 for(unsigned command:{1U,10U}){
  auto m=std::make_unique<GdnGraphTransport>(argv[1],argv[2]);m->badCommandRead=command;
  assert(!m->RunGdn() && !m->packets && !m->outputWrites);faults++;
 }
 printf("PASS QOP1 emitted ROOT transport faults%u/first-unknown inert/no later packet or CSR or scratch store/full-output late fault refused;SYNTHETIC_PEERS_NOT_NATIVE_MATH_OR_SPEED\n",faults);
}
