/* SPDX-License-Identifier: MIT */
// ACTUAL emitted entry parser:foreign semantics refuse before first NoC IO.
#define PIONEER_Q38_PROJECTION_PARENT_MACHINE_ONLY
#include "pioneer_q38_projection_graph_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;unsigned refused=0;
 const unsigned cases[][2]={{0,0x31535051},{0,0x31525051},{1,1},{1,3},{42,0},{42,3},{42,63},{42,64},{42,0xffffffff},
  {43,2},{43,4},{43,7},{43,13},{3,7},{44,6144},{45,12288},{48,12},{46,0},{16,0},{183,1},{8383,1}};
 for(const auto& bad:cases){
  auto m=std::make_unique<ProjectionParentMachine>(argv[1],1,10);
  m->memory.at(TT_Q38_PROJECTION_GRAPH_MANIFEST+bad[0]*4)=bad[1];
  if(bad[0]==46)m->memory.at(TT_Q38_PROJECTION_GRAPH_MANIFEST+47*4)=0;
  assert(!m->Run() && !m->packets && !m->nocReads && !m->producerJobs && !m->matrixJobs && !m->outputWrites && !m->unknown);
  m->Inert();refused++;
 }
 printf("PASS ACTUAL QPS2 entry semantic/old-marker/old-version/layer0/attention/vocabulary/geometry/epoch/source/padding refusals=%u BEFORE first NoC access;NO producer/native authority\n",refused);
}
