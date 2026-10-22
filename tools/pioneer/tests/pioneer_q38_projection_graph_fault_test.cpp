/* SPDX-License-Identifier: MIT */
// ACTUAL parent fault ordering,synthetic peer/transport endpoints ONLY.
#define PIONEER_Q38_PROJECTION_PARENT_MACHINE_ONLY
#include "pioneer_q38_projection_graph_image_test.cpp"
struct LateManifestParent:ProjectionParentMachine {
 bool armed=false,shadow=false,mutated=false,observedMetadata=false;
 unsigned word=0,lastOutputPacket=0;
 using ProjectionParentMachine::ProjectionParentMachine;
 void Packet()override{
  bool output=command[7]==0x212 && command[5]==fixture.graph->Manifest()[8]
   && command[0]>=0x80000 && command[0]<0x80000+119*1024;
  ProjectionParentMachine::Packet();
  if(output && !pending){lastOutputPacket=packets;if(armed){memory[(shadow?TT_Q38_PROJECTION_GRAPH_SHADOW:TT_Q38_PROJECTION_GRAPH_MANIFEST)+word*4]^=1;mutated=true;}}
 }
 uint32_t Load(uint32_t a)override{
  if(Range(a,0xffb20000,0x2000))assert(!observedMetadata);
  uint32_t value=ProjectionParentMachine::Load(a);
  if(mutated && a==(shadow?TT_Q38_PROJECTION_GRAPH_SHADOW:TT_Q38_PROJECTION_GRAPH_MANIFEST)+word*4)observedMetadata=true;
  return value;
 }
};
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);unsigned faults=0;
 for(bool shadow:{false,true})for(unsigned word:{0U,16U,40U,64U,191U,8383U}){
  auto m=std::make_unique<LateManifestParent>(argv[1]);m->armed=true;m->shadow=shadow;m->word=word;
  assert(!m->Run() && m->mutated && m->observedMetadata && m->outputWrites==1 && m->packets==m->lastOutputPacket);
  assert(m->memory.at(TT_Q38_PROJECTION_GRAPH_STATS+56*4)==0);m->Inert();faults++;
 }
 for(unsigned packet:{1U,2U,20U}){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1]);m->dropPacket=packet;
  assert(!m->Run() && m->pending && m->packets==packet && !m->producerJobs && !m->outputWrites);m->Inert();faults++;
 }
 for(unsigned read:{1U,10U,11U,20U}){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1]);m->badCommandRead=read;
  assert(!m->Run() && m->packets==(read-1)/10 && !m->producerJobs && !m->outputWrites);m->Inert();faults++;
 }
 for(unsigned field:{0U,1U,2U,3U,4U,5U,7U,8U,9U,10U,11U,12U,13U,14U,15U,44U,45U,48U,49U,63U,183U}){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1]);m->memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+field*4]^=field==3?7U:1U;
  assert(!m->Run() && !m->packets && !m->nocReads && !m->unknown);faults++;
 }
 for(unsigned kind=0;kind<3;kind++){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1]);
  if(!kind)m->memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+64*4]=101;
  if(kind==1)m->memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+(192+2)*4]=0;
  if(kind==2)m->memory[TT_Q38_PROJECTION_GRAPH_MANIFEST+(192+3*8)*4]=1;
  assert(!m->Run() && !m->packets && !m->nocReads && !m->unknown);faults++;
 }
 printf("PASS ACTUAL QPR1 phase-boundary fault cases%u:late primary/shadow corruption AFTER complete output BEFORE receipt,zero next CSR/packet;UNKNOWN/drop/readback inert;malformed data zero IO. SYNTHETIC_PEERS_NOT_NATIVE_OR_SPEED\n",faults);
}
