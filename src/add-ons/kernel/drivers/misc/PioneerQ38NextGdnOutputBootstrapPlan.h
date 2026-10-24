/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_BOOTSTRAP_PLAN_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_BOOTSTRAP_PLAN_H
#include "PioneerQ38NextGdnOutputProjectionPlan.h"
#include "PioneerQ38NextGdnOutputGraphShape.h"
// HEAP-only canonical bootstrap DATA from the SAME genuine held producer.
// Zero sequences mean REQUIRED fresh pinned-image boot,NOT permission to
// reset an existing peer or discard its history. The future native consumer
// must atomically claim the real parent/complete original snapshot and own
// image transition,initial boot,Scope/Close/END before using this manifest.
// No caller address/token/phase/source/sequence/epoch/activation hints.
class PioneerQ38NextGdnOutputBootstrapPlan {
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38NextGdnOutputProjectionPlan* input=nullptr;
 const PioneerQ38DispatchLease* cohort=nullptr;
 uint32_t manifest[TT_Q38_PROJECTION_GRAPH_WORDS]={},sources[119*16]={};
 bool attempted=false,ready=false;
 void Header(uint32_t (&d)[64])const{
  const auto& p=*input->producer;for(unsigned i=0;i<64;i++)d[i]=0;
  d[0]=TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER;d[1]=1;d[2]=67108864;d[3]=p.bank;d[4]=p.inputAddress-TT_Q38_GDN_GATED_OUTPUT;
  d[5]=1;d[6]=p.phase;d[7]=1;d[8]=PioneerQ38ProjectionRoute(p.bank);d[9]=129;
  d[10]=TT_Q38_PROJECTION_GRAPH_MANIFEST+192*4;d[11]=8;d[12]=119;d[13]=p.phase+2;d[14]=1;
  for(unsigned i=0;i<16;i++)d[16+i]=p.source[i];
  const uint64_t id[]={p.identity.session,p.identity.sequence,p.identity.modelEpoch,p.identity.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(id[i]);d[33+i*2]=uint32_t(id[i]>>32);}
  d[40]=p.position;d[41]=p.phase;d[42]=p.layer;d[43]=13;d[44]=6144;d[45]=5120;d[46]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[47]=p.token;d[48]=8;
 }
public:
 PioneerQ38NextGdnOutputBootstrapPlan()=default;
 PioneerQ38NextGdnOutputBootstrapPlan(const PioneerQ38NextGdnOutputBootstrapPlan&)=delete;
 PioneerQ38NextGdnOutputBootstrapPlan& operator=(const PioneerQ38NextGdnOutputBootstrapPlan&)=delete;
 bool Build(const PioneerQ38NextGdnOutputProjectionPlan& producer,const PioneerQ38DispatchLease& exact){
  if(attempted)return false;
  attempted=true;
  if(!producer.Current() || !producer.producer->MatchesCohort(exact))return false;
  input=&producer;cohort=&exact;
  for(unsigned w=0;w<119;w++){
   if(!exact.authority.source[w+1][0])return false;
   for(unsigned i=0;i<16;i++)sources[w*16+i]=exact.authority.source[w+1][i];
  }
  uint32_t header[64];Header(header);for(unsigned i=0;i<64;i++)manifest[i]=header[i];
  for(unsigned j=0;j<119;j++){
   const auto* part=producer.jobs.Part(j);if(!part)return false;auto* record=manifest+192+j*8;
   record[0]=producer.producer->canonical->Base+uint32_t(part->offset);record[1]=part->bank;
   record[2]=part->groups;record[3]=part->firstRow;record[4]=PioneerQ38ProjectionRoute(part->bank);
  }
  ready=true;if(Current())return true;ready=false;return false;
 }
 bool Current()const{
  if(!ready || !input || !cohort || !input->Current() || !input->producer->MatchesCohort(*cohort)
   || !PioneerQ38NextGdnOutputGraphManifest(manifest,sources))return false;
  uint32_t header[64];Header(header);for(unsigned i=0;i<64;i++)if(manifest[i]!=header[i])return false;
  for(unsigned i=64;i<192;i++)if(manifest[i])return false;
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)if(sources[w*16+i]!=cohort->authority.source[w+1][i])return false;
  for(unsigned j=0;j<119;j++){
   const auto* p=input->jobs.Part(j);const auto* r=manifest+192+j*8;if(!p)return false;
   if(r[0]!=input->producer->canonical->Base+p->offset || r[1]!=p->bank || r[2]!=p->groups || r[3]!=p->firstRow
    || r[4]!=PioneerQ38ProjectionRoute(p->bank) || r[5] || r[6] || r[7])return false;
  }
  return true; // held-only CPU relation,NOT a live execution grant
 }
 bool Matches(const PioneerQ38NextGdnOutputProjectionPlan& producer,const PioneerQ38DispatchLease& exact)const{
  return input==&producer && cohort==&exact && Current();
 }
 const uint32_t* Manifest()const{return Current()?manifest:nullptr;}
 const uint32_t* Sources()const{return Current()?sources:nullptr;}
};
static_assert(sizeof(PioneerQ38NextGdnOutputBootstrapPlan)>40960 && sizeof(PioneerQ38NextGdnOutputBootstrapPlan)<45056,"full canonical bootstrap HEAP ONLY");
#endif
