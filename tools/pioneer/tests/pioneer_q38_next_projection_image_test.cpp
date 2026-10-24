/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PROJECTION_PARENT_MACHINE_ONLY
#include "pioneer_q38_projection_graph_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned good=0,faults=0,stack=0;
 for(unsigned layer:{1U,2U,4U,61U,62U})for(unsigned role:{10U,11U,5U,6U}){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1],layer,role,good%7);assert(m->Run());m->Verify();stack=std::max(stack,0x70000-m->minStack);good++;
 }
 for(unsigned bank=0;bank<7;bank++){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1],62,11,bank,true);assert(m->Run());m->Verify();good++;
 }
 for(unsigned word=0;word<64;word++){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1],1,10);m->badNode=1;m->badProofWord=word;
  assert(!m->Run() && !m->lineReads && !m->outputWrites);m->Inert();faults++;
 }
 for(unsigned kind=0;kind<3;kind++){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1],2,6,6);if(!kind)m->badPcNode=1;if(kind==1)m->badSourceNode=1;if(kind==2)m->badCounterNode=1;
  assert(!m->Run() && !m->lineReads && !m->outputWrites);m->Inert();faults++;
 }
 for(unsigned read:{2U,4U,8U}){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1],62,5,3);m->badRootAliasRead=read;assert(!m->Run() && m->observed);m->Inert();faults++;
 }
 printf("PASS ACTUAL QPS2 ROOT+gather+matrix instructions:positives%u/faults%u/maxStack%u;all4slots/later layers/bank rotations/119STOP;UNKNOWN ZERO laterIO;SYNTHETIC PEERS_NOT_ARITHMETIC_NATIVE_OR_SPEED\n",good,faults,stack);
}
