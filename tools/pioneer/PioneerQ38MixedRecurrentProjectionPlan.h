/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_PLAN_H
#define PIONEER_Q38_MIXED_RECURRENT_PROJECTION_PLAN_H
#include "PioneerQ38MixedRecurrentProjectionRootLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
// One immutable HEAP recipe. All eight recurrent matrix roles, no caller
// coordinates/nonces/products and no original/source/native authority.
class PioneerQ38MixedRecurrentProjectionPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38ProjectionJobs jobs;uint32_t frame[64]={},header[16]={},parts[119][8]={},first[119]={},banks[8]={},sources[1920]={};
 unsigned layer=64,role=14,position=8192,token=248320,bank=7,phase=0,request=0;
 PioneerQ38GdnIdentity identity={};bool attempted=false,ready=false;
public:
 PioneerQ38MixedRecurrentProjectionPlan()=default;
 PioneerQ38MixedRecurrentProjectionPlan(const PioneerQ38MixedRecurrentProjectionPlan&)=delete;
 PioneerQ38MixedRecurrentProjectionPlan& operator=(const PioneerQ38MixedRecurrentProjectionPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selected,unsigned semanticRole,unsigned pos,unsigned inputToken,
  unsigned workspaceBank,unsigned selectedPhase,unsigned selectedRequest,const PioneerQ38GdnIdentity& id,const uint32_t* source){
  if(attempted)return false;
  attempted=true;unsigned t=0,k=0,n=0;
  if(!c.Ready() || !PioneerQ38MixedRecurrentGeometry(selected,semanticRole,t,k,n) || pos>=8192 || inputToken>=248320
   || workspaceBank>=7 || !selectedPhase || selectedPhase>=0x7fffffff || !selectedRequest || selectedRequest>=0x7fffffff
   || !id.Valid() || !source)return false;
  for(unsigned w=0;w<120;w++)if(!source[w*16])return false;
  canonical=&c;layer=selected;role=semanticRole;position=pos;token=inputToken;bank=workspaceBank;
  phase=selectedPhase;request=selectedRequest;identity=id;
  for(unsigned i=0;i<1920;i++)sources[i]=source[i];
  if(!jobs.Build(c,layer,role,position) || jobs.Waves()!=1 || jobs.Type()!=t || jobs.Inputs()!=k || jobs.Outputs()!=n)return false;
  namespace L=PioneerQ38MixedRecurrentProjectionRootLayout;
  auto* d=frame;auto* h=header;for(unsigned b=0;b<7;b++)banks[b]=PioneerQ38ProjectionRoute(b);
  d[0]=L::Magic;d[1]=1;d[2]=67108864;d[3]=bank;d[4]=c.Base+c.Workspace(bank)->offset;
  d[6]=phase;d[7]=jobs.Count();d[8]=banks[bank];d[9]=129;d[10]=PioneerQ38MixedRecurrentOffset(role);
  d[11]=n;d[12]=n*4;d[13]=L::Rows;d[14]=k;d[15]=t;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];
  const uint64_t ids[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(ids[i]);d[33+i*2]=uint32_t(ids[i]>>32);}
  d[40]=position;d[41]=token;d[42]=layer;d[43]=role;d[44]=jobs.Count();d[45]=1;
  d[48]=request;d[49]=119;d[50]=L::Parts;d[51]=L::MatrixHeader;d[52]=L::Banks;d[53]=role==13?L::Output:L::Input;
  d[54]=L::Projected;d[55]=L::PeerJobs;d[56]=L::Nonces;d[57]=L::PeerBoots;
  const unsigned fields[]={TT_Q38_MIXED_RECURRENT_MATRIX,1,1,t,k,jobs.Count(),phase,TT_Q38_DENSE_MATRIX_INPUT,1};
  for(unsigned i=0;i<9;i++)h[i]=fields[i];
  for(unsigned w=0;w<jobs.Count();w++){
   const auto* p=jobs.Part(w);auto* out=parts[w];if(!p || p->offset>UINT32_MAX-c.Base)return false;
   out[0]=c.Base+uint32_t(p->offset);out[1]=p->bank;out[2]=p->groups;out[4]=banks[p->bank];out[5]=source[(w+1)*16];first[w]=p->firstRow;
  }
  ready=true;if(Matches(c))return true;
  ready=false;return false;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  namespace L=PioneerQ38MixedRecurrentProjectionRootLayout;
  if(!ready || canonical!=&c || !jobs.Matches(c,layer,role,position) || !L::Shape(frame))return false;
  uint32_t recipe[64];for(unsigned i=0;i<64;i++)recipe[i]=i>=48?0:frame[i];
  recipe[0]=TT_Q38_MIXED_RECURRENT_MATRIX;recipe[13]=TT_Q38_DENSE_MATRIX_ROWS;
  if(!PioneerQ38MixedRecurrentRecipe(recipe,header,banks,parts,first) || frame[3]!=bank
   || frame[4]!=c.Base+c.Workspace(bank)->offset || frame[6]!=phase || frame[40]!=position || frame[41]!=token
   || frame[42]!=layer || frame[43]!=role || frame[48]!=request)return false;
  const uint64_t ids[]={identity.session,identity.sequence,identity.modelEpoch,identity.driverGeneration};
  for(unsigned i=0;i<16;i++)if(frame[16+i]!=sources[i])return false;
  for(unsigned i=0;i<4;i++)if(frame[32+i*2]!=uint32_t(ids[i]) || frame[33+i*2]!=uint32_t(ids[i]>>32))return false;
  for(unsigned w=0;w<jobs.Count();w++){
   const auto* p=jobs.Part(w);const auto* q=parts[w];
   if(q[0]!=c.Base+p->offset || q[1]!=p->bank || q[2]!=p->groups || q[5]!=sources[(w+1)*16] || first[w]!=p->firstRow)return false;
  }
  return true;
 }
 bool LearnedPagesPresent(const uint8_t* pages,unsigned count)const{
  return canonical && Matches(*canonical) && jobs.LearnedPagesPresent(pages,count);
 }
 const uint32_t* Frame()const{return ready?frame:nullptr;}
 const uint32_t* Header()const{return ready?header:nullptr;}
 const uint32_t (*Parts()const)[8]{return ready?parts:nullptr;}
 const uint32_t* Rows()const{return ready?first:nullptr;}
 const uint32_t* Banks()const{return ready?banks:nullptr;}
};
static_assert(sizeof(PioneerQ38MixedRecurrentProjectionPlan)>32768 && sizeof(PioneerQ38MixedRecurrentProjectionPlan)<65536,"canonical recurrent recipe HEAP ONLY");
#endif
