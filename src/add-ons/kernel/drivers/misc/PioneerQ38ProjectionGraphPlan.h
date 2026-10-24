/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_GRAPH_PLAN_H
#define PIONEER_Q38_PROJECTION_GRAPH_PLAN_H
#ifdef PIONEER_Q38_PROJECTION_NEXT_ROOT
#include "PioneerQ38NextProjectionGraphPlan.h"
#include "PioneerQ38NextProjectionImageAliases.h"
#else
#include "PioneerQ38ProjectionJobs.h"
#include "PioneerQ38ProjectionGraphShape.h"
#include "PioneerQ38GdnIdentity.h"
// PRIVATE HEAP DATA factory. Same canonical Jobs/plan,not caller extents.
// Source/identity/input epoch copies are DATA ONLY. A separate actual native
// owner must prove their retained objects/current physical state/producer
// and ledger under its exclusive cookie lock. This class performs NO IO.
class PioneerQ38ProjectionGraphPlan {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;
 const PioneerQ38ProjectionJobs* jobs=nullptr;
 uint32_t manifest[TT_Q38_PROJECTION_GRAPH_WORDS]={},sources[119][16]={};
 unsigned selected=65,role=14,bank=7;
 uint64_t token=0;
 bool attempted=false,ready=false;
 static void Header(uint32_t* d,const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38ProjectionJobs& jobs,
  unsigned layer,unsigned semanticRole,uint64_t position,unsigned workspaceBank,const PioneerQ38GdnIdentity& id,
  const uint32_t* rootSource,unsigned phase,uint64_t inputEpoch,bool stop){
  for(unsigned i=0;i<64;i++)d[i]=0;
  d[0]=TT_Q38_PROJECTION_GRAPH_MARKER;d[1]=1;d[2]=67108864;d[3]=workspaceBank;d[4]=canonical.Base+canonical.Workspace(workspaceBank)->offset;
  d[5]=stop?2:1;d[6]=phase;d[7]=jobs.Waves();d[8]=PioneerQ38ProjectionRoute(workspaceBank);d[9]=129;
  d[10]=TT_Q38_PROJECTION_GRAPH_MANIFEST+192*4;d[11]=8;d[12]=jobs.Count();d[13]=phase+(stop?1:jobs.Waves()+1);d[14]=stop?0:1;
  for(unsigned i=0;i<16;i++)d[16+i]=rootSource[i];
  const uint64_t identity[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(identity[i]);d[33+i*2]=uint32_t(identity[i]>>32);}
  d[40]=uint32_t(position);d[41]=uint32_t(position>>32);d[42]=layer;d[43]=semanticRole;d[44]=jobs.Inputs();d[45]=jobs.Outputs();
  d[46]=uint32_t(inputEpoch);d[47]=uint32_t(inputEpoch>>32);d[48]=jobs.Type();
 }
public:
 PioneerQ38ProjectionGraphPlan()=default;
 PioneerQ38ProjectionGraphPlan(const PioneerQ38ProjectionGraphPlan&)=delete;
 PioneerQ38ProjectionGraphPlan& operator=(const PioneerQ38ProjectionGraphPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38ProjectionJobs& canonicalJobs,
  unsigned layer,unsigned semanticRole,uint64_t position,unsigned workspaceBank,const PioneerQ38GdnIdentity& id,
  const uint32_t* rootSource,const uint32_t peerSource[119][16],const uint32_t* sequence,unsigned phase,uint64_t inputEpoch,bool stop){
  if(attempted)return false;
  attempted=true;
  if(!id.Valid() || !rootSource || !peerSource || !sequence || !rootSource[0] || workspaceBank>=7
   || !canonicalJobs.Matches(canonical,layer,semanticRole,position) || (stop?inputEpoch!=0:inputEpoch==0))return false;
  const auto* workspace=canonical.Workspace(workspaceBank);if(!workspace || workspace->bytes!=67108864)return false;
  plan=&canonical;jobs=&canonicalJobs;selected=layer;role=semanticRole;bank=workspaceBank;token=position;
  auto* d=manifest;Header(d,canonical,canonicalJobs,layer,semanticRole,position,workspaceBank,id,rootSource,phase,inputEpoch,stop);
  for(unsigned w=0;w<119;w++){d[64+w]=sequence[w];for(unsigned i=0;i<16;i++)sources[w][i]=peerSource[w][i];}
  for(unsigned i=0;i<jobs->Count();i++){
   const auto& p=*jobs->Part(i);auto* r=d+192+i*8;
   r[0]=canonical.Base+uint32_t(p.offset);r[1]=p.bank;r[2]=p.groups;r[3]=p.firstRow;r[4]=PioneerQ38ProjectionRoute(p.bank);
  }
  ready=PioneerQ38ProjectionGraphManifest(manifest,sources[0]);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38ProjectionJobs& canonicalJobs,
  unsigned layer,unsigned semanticRole,uint64_t position,unsigned workspaceBank,const PioneerQ38GdnIdentity& id,
  const uint32_t* rootSource,const uint32_t peerSource[119][16],const uint32_t* sequence,unsigned phase,uint64_t inputEpoch,bool stop)const{
  if(!ready || plan!=&canonical || jobs!=&canonicalJobs || selected!=layer || role!=semanticRole || token!=position
   || bank!=workspaceBank || !id.Valid() || !rootSource || !peerSource || !sequence
   || !canonicalJobs.Matches(canonical,layer,semanticRole,position) || !PioneerQ38ProjectionGraphManifest(manifest,sources[0]))return false;
  const auto* workspace=canonical.Workspace(bank);if(!workspace || manifest[4]!=canonical.Base+workspace->offset)return false;
  uint32_t header[64];Header(header,canonical,canonicalJobs,layer,semanticRole,position,workspaceBank,id,rootSource,phase,inputEpoch,stop);
  for(unsigned i=0;i<64;i++)if(header[i]!=manifest[i])return false;
  for(unsigned w=0;w<119;w++){
   if(manifest[64+w]!=sequence[w])return false;
   for(unsigned i=0;i<16;i++)if(sources[w][i]!=peerSource[w][i])return false;
  }
  for(unsigned i=0;i<canonicalJobs.Count();i++){
   const auto& p=*canonicalJobs.Part(i);const auto* r=manifest+192+i*8;
   if(r[0]!=canonical.Base+p.offset || r[1]!=p.bank || r[2]!=p.groups || r[3]!=p.firstRow || r[4]!=PioneerQ38ProjectionRoute(p.bank))return false;
  }
  return true;
 }
 const uint32_t* Manifest()const{return ready?manifest:nullptr;}
 const uint32_t* Sources()const{return ready?sources[0]:nullptr;}
};
static_assert(sizeof(PioneerQ38ProjectionGraphPlan)>40960 && sizeof(PioneerQ38ProjectionGraphPlan)<45056,"complete projection graph DATA is HEAP ONLY");
#endif // separate QPS2 image grammar; default definitions remain unchanged
#endif
