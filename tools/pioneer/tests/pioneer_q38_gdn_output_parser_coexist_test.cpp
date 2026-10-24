/* SPDX-License-Identifier: MIT */
// Strict metadata parsers ONLY. Same binary with/without old stash mode.
#define PIONEER_Q38_PROJECTION_GRAPH_PLAN_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_plan_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphShape.h"
struct MatrixParts{uint32_t word[119][8]={};};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 ParentPlanFixture f(0,13);auto m=std::make_unique<uint32_t[]>(TT_Q38_PROJECTION_GRAPH_WORDS);
 m[0]=TT_Q38_GDN_OUTPUT_GRAPH_MARKER;m[1]=1;m[2]=67108864;m[3]=f.bank;m[4]=f.canonical->Base+f.canonical->Workspace(f.bank)->offset;
 m[5]=1;m[6]=16;m[7]=1;m[8]=PioneerQ38ProjectionRoute(f.bank);m[9]=129;m[10]=TT_Q38_PROJECTION_GRAPH_MANIFEST+192*4;
 m[11]=8;m[12]=119;m[13]=18;m[14]=1;
 for(unsigned i=0;i<16;i++)m[16+i]=f.root[i];
 for(unsigned i=0;i<4;i++)m[32+i*2]=i+1;
 m[41]=16;m[43]=13;m[44]=6144;m[45]=5120;m[46]=TT_Q38_GDN_RENDEZVOUS_MARKER;m[47]=248319;m[48]=8;
 auto parts=std::make_unique<MatrixParts>();uint32_t rows[119],banks[8]={},h[16]={},d[64]={};
 for(unsigned b=0;b<7;b++)banks[b]=PioneerQ38ProjectionRoute(b);
 for(unsigned w=0;w<119;w++){
  const auto& p=*f.jobs->Part(w);auto* r=m.get()+192+w*8;
  r[0]=f.canonical->Base+uint32_t(p.offset);r[1]=p.bank;r[2]=p.groups;r[3]=p.firstRow;r[4]=banks[p.bank];
  for(unsigned i=0;i<8;i++)parts->word[w][i]=i==3?1:i==5?f.sources[w][0]:r[i];rows[w]=p.firstRow;
 }
 assert(PioneerQ38GdnOutputGraphManifest(m.get(),f.sources[0]));
 for(unsigned i=16;i<44;i++)d[i]=m[i];
 d[0]=TT_Q38_GDN_OUTPUT_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=f.bank;d[4]=m[4];d[6]=18;d[7]=119;d[8]=m[8];d[9]=129;
 d[10]=TT_Q38_GDN_OUTPUT_RESULT;d[11]=5120;d[12]=20480;d[13]=TT_Q38_PROJECTION_MATRIX_ROWS;d[14]=6144;d[15]=8;
 d[44]=119;d[45]=1;d[46]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[47]=248319;
 h[0]=d[0];h[1]=h[2]=1;h[3]=8;h[4]=6144;h[5]=119;h[6]=18;h[7]=TT_Q38_PROJECTION_MATRIX_INPUT;h[8]=1;
 auto valid=[&](){return PioneerQ38GdnOutputMatrixShape(d,h,banks,parts->word,rows);};assert(valid());unsigned faults=0;
 for(unsigned i:{0U,1U,2U,3U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U,40U,41U,42U,43U,44U,45U,46U,48U,63U}){
  d[i]^=1;assert(!valid());d[i]^=1;faults++;
 }
 for(unsigned i=0;i<16;i++){h[i]^=1;assert(!valid());h[i]^=1;faults++;}
 for(unsigned w=0;w<119;w++){
  unsigned saved=rows[w];rows[w]=w?rows[0]:rows[1];assert(!valid());rows[w]=saved;faults++;
 }
 d[12]-=64;assert(!valid());d[12]+=64;faults++;
 assert(valid());
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=m[i];old[0]=TT_Q38_PROJECTION_GRAPH_MARKER;
 assert(!PioneerQ38ProjectionGraphFrame(old) && !f.Build()); // cannot widen old RMS-only role13
 d[0]=h[0]=TT_Q38_PROJECTION_MATRIX_MARKER;d[10]=TT_Q38_PROJECTION_OUTPUT_OFFSET;d[46]=d[47]=0;
 assert(!PioneerQ38ProjectionMatrixShape(d,h,banks,parts->word,rows));
#endif
 printf("PASS independent QOP1/QOM1 strict complete5120row parsers/faults%u/old stash mode%u;NO RMS role widening or native grant\n",faults,
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
  1U
#else
  0U
#endif
 );
}
