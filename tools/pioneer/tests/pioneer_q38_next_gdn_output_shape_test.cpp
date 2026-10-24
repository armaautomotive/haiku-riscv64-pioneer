/* SPDX-License-Identifier: MIT */
// Canonical geometry plus SYNTHETIC ancestry DATA only. These constructors
// are TEST-only, not native input/source/completion or launch capabilities.
#define PIONEER_Q38_PROJECTION_GRAPH_PLAN_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_plan_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextGdnOutputGraphProof.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphShape.h"
#include <array>
#include <vector>
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 unsigned frames=0,inputs=0,refusals=0,records=0;
 for(unsigned layer=1;layer<64;layer++)if(PioneerQ38NextGdnOutputLayer(layer))
 for(unsigned position:{0U,8191U})for(unsigned bank=0;bank<7;bank++)for(bool stop:{false,true}){
  ParentPlanFixture f(layer,13,bank,stop);f.token=position;
  f.jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(f.jobs->Build(*f.canonical,layer,13,position) && f.Build());
  const auto* original=f.graph->Manifest();std::vector<uint32_t> m(original,original+TT_Q38_PROJECTION_GRAPH_WORDS);
  m[0]=TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER;m[41]=f.phase;
  m[46]=stop?0:TT_Q38_GDN_RENDEZVOUS_MARKER;m[47]=stop?0:248319;
  assert(PioneerQ38NextGdnOutputGraphManifest(m.data(),f.sources[0]) && !PioneerQ38GdnOutputGraphFrame(m.data()));
  assert(PioneerQ38NextGdnOutputResultOffset(layer,13)==TT_Q38_NEXT_GDN_OUTPUT_RESULT);
  for(unsigned ordinal=0;ordinal<119;ordinal++){
   const auto& p=*f.jobs->Part(ordinal);const auto* r=m.data()+192+ordinal*8;
   assert(r[0]==f.canonical->Base+p.offset && r[1]==p.bank && r[2]==p.groups && r[3]==p.firstRow);records++;
  }
  for(unsigned badLayer:{0U,3U,63U,64U,UINT32_MAX}){
   auto d=m;d[42]=badLayer;assert(!PioneerQ38NextGdnOutputGraphFrame(d.data()));refusals++;
  }
  for(unsigned badPosition:{8192U,UINT32_MAX}){
   auto d=m;d[40]=badPosition;assert(!PioneerQ38NextGdnOutputGraphFrame(d.data()));refusals++;
  }
  auto cross=m;cross[0]=TT_Q38_GDN_OUTPUT_GRAPH_MARKER;
  assert(!PioneerQ38NextGdnOutputGraphFrame(cross.data()) && !PioneerQ38GdnOutputGraphFrame(cross.data()));refusals++;
  frames++;if(stop)continue;
  uint32_t g[64]={};g[0]=TT_Q38_NEXT_GDN_OUTPUT_GATHER_MARKER;g[1]=1;g[2]=67108864;g[3]=bank;g[4]=m[4];g[5]=24;g[6]=f.phase+1;
  g[8]=m[8];g[9]=129;g[10]=TT_Q38_PROJECTION_NATIVE_OFFSET;g[11]=TT_Q38_PROJECTION_LINES_OFFSET;
  g[12]=TT_Q38_PROJECTION_COMPACT_OFFSET;g[13]=12288;g[14]=768;g[15]=8;
  for(unsigned i=16;i<44;i++)g[i]=m[i];g[44]=m[46];g[45]=m[47];g[46]=TT_Q38_NEXT_GDN_OUTPUT_GATHER_SOURCES;g[47]=24;
  assert(PioneerQ38NextGdnOutputGatherShape(g) && !PioneerQ38GdnOutputGatherShape(g));
  for(unsigned chunk=0;chunk<24;chunk++){
   uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=g[i];d[0]=TT_Q38_NEXT_GDN_OUTPUT_INPUT_MARKER;d[5]=chunk;d[7]=chunk+1;
   d[9]=PioneerQ38ProjectionPeer(chunk);d[10]=TT_Q38_GDN_GATED_OUTPUT+chunk*1024;d[11]=6144;
   d[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+chunk*512;d[13]=TT_Q38_PROJECTION_LINES_OFFSET+chunk*128;d[14]=8;d[15]=24;d[46]=d[47]=0;
   for(unsigned i=0;i<16;i++)d[16+i]=f.sources[chunk][i];
   assert(PioneerQ38NextGdnOutputInputShape(d) && !PioneerQ38GdnOutputInputShape(d));
   uint32_t line[32]={};const uint32_t fields[]={TT_Q38_NEXT_GDN_OUTPUT_INPUT_MARKER,d[6],chunk,8,6144,position,13,layer,f.phase};
   for(unsigned i=0;i<9;i++)line[8+i]=fields[i];for(unsigned i=0;i<8;i++)line[17+i]=d[32+i];
   line[25]=24;line[26]=chunk+1;line[27]=d[9];line[28]=d[16];line[29]=TT_Q38_GDN_RENDEZVOUS_MARKER;line[30]=248319;line[31]=1;
   assert(PioneerQ38NextGdnOutputInputLine(line,d) && PioneerQ38NextGdnOutputGatherLine(line,g,chunk,f.sources[chunk]));
   // A well-formed but foreign position/layer remains DATA. Its receipt
   // cannot match the unchanged descriptor or become a native grant.
   for(unsigned word:{13U,15U,16U,30U}){line[word]^=1;assert(!PioneerQ38NextGdnOutputInputLine(line,d)
    && !PioneerQ38NextGdnOutputGatherLine(line,g,chunk,f.sources[chunk]));line[word]^=1;refusals++;}
   inputs++;
  }
  if(layer==1 && !position && !bank){
   for(unsigned ordinal=0;ordinal<119;ordinal++)for(unsigned word=0;word<8;word++){
    m[192+ordinal*8+word]^=1;assert(!PioneerQ38NextGdnOutputGraphManifest(m.data(),f.sources[0]));m[192+ordinal*8+word]^=1;refusals++;
   }
   // Coherent foreign extents and nonzero source metadata can have valid
   // DATA shape. Only the future real owner can authenticate those facts.
   unsigned selected=m[193];for(unsigned w=0;w<119;w++)if(m[192+w*8+1]==selected)m[192+w*8]+=64;
   assert(PioneerQ38NextGdnOutputGraphManifest(m.data(),f.sources[0]));
  }
 }
 for(unsigned layer:{0U,3U,63U,64U,UINT32_MAX})assert(PioneerQ38NextGdnOutputResultOffset(layer,13)==UINT32_MAX);
 for(unsigned role=0;role<14;role++)if(role!=13)assert(PioneerQ38NextGdnOutputResultOffset(1,role)==UINT32_MAX);
 printf("PASS QOP2/QOI2/QOG2 DATA frames%u/canonicalRecords%u/inputLines%u/refusals%u/ALL47nonzero recurrent layers/positions0+8191/ALL7banks/RUN+STOP/foreign DATA never native authority;NO_IMAGE_HARDWARE_OR_SPEED_PROOF\n",frames,records,inputs,refusals);
}
