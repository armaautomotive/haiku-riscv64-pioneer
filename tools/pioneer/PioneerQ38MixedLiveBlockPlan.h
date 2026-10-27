/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_BLOCK_PLAN_H
#define PIONEER_Q38_MIXED_LIVE_BLOCK_PLAN_H
#include "PioneerQ38MixedLiveBlockRootLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38DenseProjectionPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BlockFfnRecipe.h"
// Canonical immutable command DATA for the actual QBL1 program. No caller
// weight coordinates, recipes, routes, expected products or live nonces.
// Input coordinates are DATA ONLY: a genuine native owner must separately
// bind a completed device producer, retained source, initialization and all
// L1/NoC/Tensix/GDDR/clock/host lifetimes. This class performs NO device IO.
// New KV convention is explicit: rows0..3=K groups0..3, rows4..7=V groups.
// It does not reinterpret or publish an existing cache/committed token.
class PioneerQ38MixedLiveBlockPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38DenseProjectionPlan projections;
 PioneerQ38ProjectionJobs ffn[3]; // HEAP members,never a25KiB kernel local
 uint32_t outer[128]={},boot[120][64]={},dense[96][64]={},recipes[7][1280]={};
 uint32_t boundOuter[128]={};
 bool attempted=false,ready=false;
 static const PioneerQ38CanonicalPart* Weight(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned role,unsigned bytes){
  const PioneerQ38CanonicalPart* found=nullptr;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.WeightKind || p->layer!=layer || p->role!=role)continue;
   if(found || p->type || p->firstRow || p->rows!=1 || p->rowBytes!=bytes || p->bytes!=bytes)return nullptr;
   found=p;
  }
  return found;
 }
 static bool Kv(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned row,unsigned& bank,unsigned& address){
  bool found=false;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.KvKind || p->layer!=layer || row<p->firstRow || row-p->firstRow>=p->rows)continue;
   if(found || p->role || p->type || p->rowBytes!=4194304 || p->bank>=7)return false;
   bank=p->bank;address=c.Base+p->offset+(row-p->firstRow)*p->rowBytes;found=true;
  }
  return found;
 }
 static bool DenseKv(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned (&banks)[8],unsigned (&addresses)[8]){
  // A Shape/Build call needs eight distinct rows,not two new full-model
  // scans for each of its96 replicated descriptors. Revalidate EVERY row
  // on EVERY call;these are stack locals,NEVER a persistent ready cache.
  for(unsigned g=0;g<4;g++)if(!Kv(c,layer,g,banks[g],addresses[g]) || !Kv(c,layer,g+4,banks[g+4],addresses[g+4]))return false;
  return true;
 }
 static void Pack(uint32_t* out,const uint32_t* d,const uint32_t* h,const uint32_t (*p)[8],const uint32_t* rows,const uint32_t* banks){
  for(unsigned i=0;i<64;i++)out[i]=d[i];
  for(unsigned i=0;i<16;i++)out[64+i]=h[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<8;i++)out[80+w*8+i]=p[w][i];
  for(unsigned w=0;w<119;w++)out[1032+w]=rows[w];
  for(unsigned i=0;i<8;i++)out[1151+i]=banks[i];
 }
 bool ExactRecipes()const{
  namespace L=PioneerQ38MixedLiveBlockRootLayout;
  for(unsigned slot=0;slot<7;slot++){
   const auto* r=recipes[slot];
   for(unsigned i=0;i<64;i++){
    unsigned expected=projections.Frame(slot<4?slot:0)[i];
    if(slot>=4){
     unsigned n=slot-4;const auto& j=ffn[n];
     switch(i){case 0:expected=L::MatrixMagic;break;case 6:expected=outer[6]+5+n;break;case 7:case 44:expected=119;break;
      case 10:expected=L::Offset(n+2);break;case 11:expected=j.Outputs();break;case 12:expected=j.Outputs()*4;break;
      case 14:expected=j.Inputs();break;case 15:expected=12;break;case 43:expected=n+2;break;default:break;}
    }
    if(r[i]!=expected)return false;
   }
   for(unsigned i=0;i<16;i++){
    unsigned expected=slot<4?projections.Header(slot)[i]:i<9?(i==0?unsigned(L::MatrixMagic):i==1 || i==2 || i==8?1U:i==3?12U:
     i==4?ffn[slot-4].Inputs():i==5?119U:i==6?outer[6]+slot+1:unsigned(TT_Q38_DENSE_MATRIX_INPUT)):0;
    if(r[64+i]!=expected)return false;
   }
   for(unsigned w=0;w<119;w++){
    const auto* p=slot<4?nullptr:ffn[slot-4].Part(w);
    if(slot>=4 && !p)return false;
    for(unsigned i=0;i<8;i++){
     unsigned expected=slot<4?projections.Parts(slot)[w][i]:i==0?canonical->Base+uint32_t(p->offset):i==1?p->bank:i==2?p->groups:
      i==4?PioneerQ38ProjectionRoute(p->bank):i==5?projections.Source(w+1)[0]:0;
     if(r[80+w*8+i]!=expected)return false;
    }
    if(r[1032+w]!=(slot<4?projections.Rows(slot)[w]:p->firstRow))return false;
   }
   for(unsigned i=0;i<8;i++)if(r[1151+i]!=projections.Banks()[i])return false;
  }
  return true;
 }
 bool DescribeDense(unsigned g,unsigned o,uint32_t* d,const unsigned (&banks)[8],const unsigned (&addresses)[8])const{
  namespace W=PioneerQ38DensePartitionLayout;
  if(g>=4 || o>=24)return false;
  unsigned kb=banks[g],vb=banks[g+4],ka=addresses[g],va=addresses[g+4];
  for(unsigned i=0;i<64;i++)d[i]=0;
  unsigned w=g*24+o,position=outer[4];
  const unsigned h[]={W::Magic,1,64,outer[3],position,g,o,position+1,ka,va,kb,vb,W::Route(kb),W::Route(vb),W::Peer(w+1),w+1};
  for(unsigned i=0;i<16;i++){d[i]=h[i];d[16+i]=projections.Source(w+1)[i];}
  for(unsigned i=0;i<8;i++)d[32+i]=projections.Frame(0)[32+i];
  d[40]=outer[6]+3;d[41]=256;d[42]=0x3d800000;d[43]=32;d[44]=8192;d[45]=16384;
  d[48]=W::First(position+1,o);d[49]=W::Count(position+1,o);
  unsigned prior=d[48]>=position?0:position-d[48];if(prior>d[49])prior=d[49];
  d[46]=2*((prior+31)>>5);d[47]=prior*16;d[50]=3;d[51]=6;d[52]=24;d[53]=384;d[54]=1536;d[55]=W::Magic;
  return W::Shape(d);
 }
 bool Shape()const{
  namespace L=PioneerQ38MixedLiveBlockRootLayout;
  namespace M=PioneerQ38MixedWorkerLayout;
  if(!canonical || !canonical->Ready() || !L::RunShape(outer) || !L::BootShape(boot[0]) || !projections.Matches(*canonical))return false;
  for(unsigned i=0;i<128;i++)if(outer[i]!=boundOuter[i])return false;
  for(unsigned w=0;w<120;w++){
   if(w && !M::BootShape(boot[w]))return false;
   const unsigned head[]={w?unsigned(M::Magic):unsigned(L::Magic),1,64,w?w:129,w?M::Peer(w):projections.Source(0)[0],w?projections.Source(w)[0]:119,8192,64};
   for(unsigned i=0;i<64;i++){
    unsigned expected=i<8?head[i]:i<16?projections.Frame(0)[24+i]:i<32?projections.Source(w)[i-16]:0;
    if(boot[w][i]!=expected)return false;
   }
  }
  for(unsigned slot=0;slot<3;slot++)if(!ffn[slot].Matches(*canonical,outer[3],slot+2,outer[4]))return false;
  if(!ExactRecipes())return false;
  for(unsigned slot=0;slot<7;slot++){
   const auto* r=recipes[slot];
   if(slot<4){
    if(!PioneerQ38DenseProjectionRecipe(r,r+64,r+1151,reinterpret_cast<const uint32_t(*)[8]>(r+80),r+1032,false))return false;
   }else if(!PioneerQ38BlockFfnRecipe(r,r+64,r+1151,reinterpret_cast<const uint32_t(*)[8]>(r+80),r+1032))return false;
   for(unsigned i=1159;i<1280;i++)if(r[i])return false;
  }
  unsigned kvBanks[8],kvAddresses[8];if(!DenseKv(*canonical,outer[3],kvBanks,kvAddresses))return false;
  for(unsigned w=0;w<96;w++){
   uint32_t expected[64];if(!DescribeDense(w/24,w%24,expected,kvBanks,kvAddresses))return false;
   for(unsigned i=0;i<64;i++)if(dense[w][i]!=expected[i])return false;
  }
  return true;
 }
public:
 PioneerQ38MixedLiveBlockPlan()=default;
 PioneerQ38MixedLiveBlockPlan(const PioneerQ38MixedLiveBlockPlan&)=delete;
 PioneerQ38MixedLiveBlockPlan& operator=(const PioneerQ38MixedLiveBlockPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned layer,unsigned position,unsigned token,unsigned bank,unsigned phase,unsigned request,
  const PioneerQ38GdnIdentity& id,const uint32_t* sources,unsigned inputBank,uint32_t inputAddress){
  namespace L=PioneerQ38MixedLiveBlockRootLayout;
  namespace M=PioneerQ38MixedWorkerLayout;
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || layer>=64 || layer%4!=3 || position>=8192 || token>=248320 || bank>=7 || !phase || phase>=0x7ffffff7
   || !request || request>=0x7fffffff || !id.Valid() || !sources || inputBank>=7 || inputAddress<c.Base
   || inputAddress>uint64_t(c.Base)+c.Capacity-20480 || (inputAddress&63))return false;
  for(unsigned w=0;w<120;w++)if(!sources[w*16])return false;
  if(!projections.Build(c,layer,position,token,bank,phase,id,sources))return false;
  canonical=&c;
  const uint64_t ids[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned w=0;w<120;w++){
   auto* b=boot[w];const unsigned head[]={w?unsigned(M::Magic):unsigned(L::Magic),1,64,w?w:129,w?M::Peer(w):sources[0],w?sources[w*16]:119,8192,64};
   for(unsigned i=0;i<8;i++){b[i]=head[i];b[8+i]=uint32_t(ids[i/2]>>(i%2*32));}
   for(unsigned i=0;i<16;i++)b[16+i]=sources[w*16+i];
  }
  const auto* workspace=c.Workspace(bank);
  const unsigned head[]={L::Magic,1,128,layer,position,129,phase,bank,PioneerQ38ProjectionRoute(bank),c.Base+workspace->offset,5120,5120};
  for(unsigned i=0;i<12;i++)outer[i]=head[i];
  for(unsigned i=0;i<16;i++)outer[16+i]=sources[i];
  for(unsigned i=0;i<8;i++)outer[32+i]=boot[0][8+i];
  const unsigned tail[]={outer[9]+L::RopeOffset,outer[8],8192,64,L::Recipes,L::RecipeCopies,L::DenseCopies,L::RawCopy,request,119,L::Frames,L::Raw,token,5120,67108864,
   L::Original,L::OriginalCopy,L::FfnRecipes,L::FfnCopies,L::Final};
  for(unsigned i=0;i<20;i++)outer[40+i]=tail[i];
  for(unsigned slot=0;slot<4;slot++){
   unsigned role=slot<2?slot:slot+6;
   const auto* p=Weight(c,layer,role,role<2?20480:1024);if(!p)return false;
   unsigned at=role<2?64+role*2:12+(role-8)*2;outer[at]=p->bank;outer[at+1]=c.Base+p->offset;
  }
  outer[68]=outer[9]+L::MetaOffset;outer[69]=outer[8];outer[70]=3;outer[71]=L::RecipeStride;
  outer[72]=L::Run;outer[73]=inputBank;outer[74]=inputAddress;outer[75]=5120;
  for(unsigned slot=0;slot<4;slot++)Pack(recipes[slot],projections.Frame(slot),projections.Header(slot),projections.Parts(slot),projections.Rows(slot),projections.Banks());
  for(unsigned slot=0;slot<3;slot++){
   auto& jobs=ffn[slot];if(!jobs.Build(c,layer,slot+2,position) || jobs.Count()!=119 || jobs.Waves()!=1 || jobs.Type()!=12)return false;
   auto* d=recipes[4+slot];for(unsigned i=0;i<64;i++)d[i]=recipes[0][i];
   d[0]=L::MatrixMagic;d[6]=phase+5+slot;d[7]=119;d[10]=L::Offset(slot+2);d[11]=jobs.Outputs();d[12]=d[11]*4;
   d[14]=jobs.Inputs();d[15]=12;d[43]=slot+2;d[44]=119;
   const unsigned h[]={L::MatrixMagic,1,1,12,jobs.Inputs(),119,d[6],TT_Q38_DENSE_MATRIX_INPUT,1};
   for(unsigned i=0;i<9;i++)d[64+i]=h[i];
   for(unsigned b=0;b<7;b++)d[1151+b]=PioneerQ38ProjectionRoute(b);
   for(unsigned w=0;w<119;w++){
    const auto* p=jobs.Part(w);auto* q=d+80+w*8;
    q[0]=c.Base+p->offset;q[1]=p->bank;q[2]=p->groups;q[4]=PioneerQ38ProjectionRoute(p->bank);q[5]=sources[(w+1)*16];d[1032+w]=p->firstRow;
   }
  }
  unsigned kvBanks[8],kvAddresses[8];if(!DenseKv(c,layer,kvBanks,kvAddresses))return false;
  for(unsigned w=0;w<96;w++)if(!DescribeDense(w/24,w%24,dense[w],kvBanks,kvAddresses))return false;
  for(unsigned i=0;i<128;i++)boundOuter[i]=outer[i];
  ready=Shape();return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{return ready && canonical==&c && Shape();}
 // ALL eleven learned tensors,not only the seven matrices. Exact byte1 in
 // the retained upload ledger is necessary, never source authentication.
 bool LearnedPagesPresent(const uint8_t* ledger,unsigned pages)const{
  if(!ready || !canonical || !Matches(*canonical) || !ledger || pages!=PioneerResidentBanks::Pages)return false;
  unsigned tensors=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer!=outer[3])continue;
   unsigned base=p->bank*unsigned(canonical->Capacity/4096)+p->offset/4096;
   if(!p->firstRow)tensors++;
   for(unsigned j=0;j<p->allocated/4096;j++)if(ledger[base+j]!=1)return false;
  }
  return tensors==11;
 }
 const uint32_t* Outer()const{return ready?outer:nullptr;}
 const uint32_t* Boot(unsigned w)const{return ready && w<120?boot[w]:nullptr;}
 const uint32_t* Recipe(unsigned slot)const{return ready && slot<7?recipes[slot]:nullptr;}
 const uint32_t* Dense(unsigned w)const{return ready && w<96?dense[w]:nullptr;}
};
static_assert(sizeof(PioneerQ38MixedLiveBlockPlan)>262144 && sizeof(PioneerQ38MixedLiveBlockPlan)<393216,"complete immutable program DATA is HEAP ONLY");
#endif
