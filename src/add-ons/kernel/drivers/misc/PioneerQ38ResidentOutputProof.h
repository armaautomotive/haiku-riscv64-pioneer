/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_PROOF_H
#define PIONEER_Q38_RESIDENT_OUTPUT_PROOF_H
#include "PioneerQ38ResidentOutputCohortShape.h"
#include "PioneerQ38RootCycleData.h"
#include "PioneerQ38PeerTerminal.h"
// DATA checks for the complete NEW resident pipeline, not an IO/cleanup,
// initialized-slot or producer capability. A genuine executing owner MUST
// capture independent physical baselines BEFORE release, retain exact
// code/model/clock/sources/hosts, and read these receipts while unheld.
// In particular neither a copied receipt nor a public result can authorize
// reset/restore. No CPU learned arithmetic or numeric reference is here.
static inline unsigned PioneerQ38ResidentOutputSequence(const uint32_t* m,unsigned w){
 if(!m || !w || w>=120 || m[64+w-1]>=0x7ffffffb)return UINT32_MAX;
 return m[64+w-1]+2+unsigned(w<=96)+unsigned(w<=24);
}
static inline uint32_t PioneerQ38ResidentOutputDelta(unsigned i,uint32_t reads,uint32_t writes,uint32_t flits,uint32_t writeFlits){
 return i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
}
static inline bool PioneerQ38ResidentOutputRootTerminalPc(uint32_t pc){return pc==0x72038 || pc==0x7203c;}
static inline bool PioneerQ38ResidentOutputRootRejectedPc(uint32_t pc){return pc>=0x72040 && pc<0x72080;}
static inline bool PioneerQ38ResidentOutputRootPhysical(const uint32_t* status,const uint32_t* pc,uint32_t reset){
 // Only for the NEW complete pinned image:both72038/7203c branch to safe
 // PARK;fault is separately72060. Full installed-image checks remain owed
 // by the executing owner BEFORE any hold/restore,not by copied CPU data.
 return status && pc && reset==0x46800 && status[0]==0x7200c && status[1]==5120
  && status[2]==TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER
  && PioneerQ38ResidentOutputRootTerminalPc(pc[0]) && PioneerQ38ResidentOutputRootTerminalPc(pc[1]);
}
static inline bool PioneerQ38ResidentOutputPeerPhysical(const uint32_t* m,unsigned w,const uint32_t* source,
 const uint32_t* boot,const uint32_t* status,uint32_t doorbell,const uint32_t* pc,uint32_t reset,bool stop){
 if(!m || !w || w>=120 || !source || !boot || !status || !pc || reset!=0x46800)return false;
 const unsigned previous=PioneerQ38ResidentOutputSequence(m,w);
 if(previous==UINT32_MAX || previous>=0x7ffffffe)return false;
 const unsigned sequence=previous+unsigned(stop),route=PioneerQ38ConvPeer(w);
 for(unsigned i=0;i<16;i++)if(source[i]!=PioneerQ38CohortSource(m,w)[i]
  || status[i]!=(i<2?sequence:i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==4?unsigned(stop)*2:0))return false;
 for(unsigned i=0;i<8;i++)if(boot[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:i==2?route:i==3?source[0]:0))return false;
 for(unsigned i=0;i<2;i++)if(stop?!PioneerQ38PeerTerminalPc(pc[i]):pc[i]!=0x7204c && pc[i]!=0x72050)return false;
 return doorbell==sequence;
}
static inline bool PioneerQ38ResidentOutputRootReceipt(const uint32_t* m,const uint32_t* q,const uint32_t* before,
 const uint32_t* after,const uint32_t* s,const uint32_t* matrix,const uint32_t* command){
 if(!before || !after || !s || !matrix || !command || !PioneerQ38ResidentCohortShape(m)
  || !PioneerQ38ResidentOutputCohortShape(q,m) || before[2]!=before[6] || after[2]!=after[6])return false;
 // 716 bounded WAIT invocations. Each poll reads status and at most three
 // physical PC/reset registers; each successful invocation owes all three.
 // Fixed packet totals are derived from ALL stages, not fitted to a trace.
 const uint64_t waits=s[4],reads=s[0];
 if(waits<716 || waits>UINT64_C(716)*65536 || reads<18022+waits+3*716 || reads>18022+4*waits
  || (reads-18022-waits)%3 || s[1]!=1005 || uint64_t(s[2])!=reads+1124 || s[3]!=4678
  || s[5]!=120 || s[6]!=96 || s[7]!=24 || s[8]!=24 || s[9]!=5120)return false;
 for(unsigned i=10;i<64;i++)if(s[i])return false;
 const auto* a=matrix;
 if(a[10]<119 || a[10]>119*65536U || a[11]<119 || a[11]>a[10] || !PioneerQ38RootCycleDataValid(a))return false;
 const uint32_t mr=a[10]+3*a[11]+121,mf=a[10]+3*a[11]+1000;
 for(unsigned i=0;i<64;i++){
  uint32_t expected=0;
  if(i==0 || i==49)expected=TT_Q38_PROJECTION_MATRIX_MARKER;
  else if(i==1 || i==2 || i==48 || i==56)expected=1;
  else if(i==3)expected=8;else if(i==4)expected=6144;else if(i==5 || i==52)expected=119;
  else if(i==6)expected=mr;else if(i==7)expected=357;else if(i==8)expected=mf;else if(i==9)expected=558;
  else if(i==10 || i==11 || (i>=16 && i<32) || i>=57)continue;
  else if(i==12)expected=q[7];else if(i>=32 && i<48)expected=m[16+i-32];
  else if(i==50)expected=2;else if(i==51)expected=204;else if(i==53)expected=320;else if(i==55)expected=5120;
  if(a[i]!=expected)return false;
 }
 // Native ROOT contributes six Conv groups; gather is24 line reads+one
 // compact write. Matrix traffic has its own exact independent receipt.
 const unsigned conv[]={18,48,30,18,12,24,30,12},gather[]={24,48,25,24,1,12,25,1};
 const uint32_t postReads=after[0]-a[24];
 if(postReads<28*119U || postReads>(24+4*65536U)*119)return false;
 for(unsigned i=0;i<8;i++){
  const uint32_t md=PioneerQ38ResidentOutputDelta(i,mr,357,mf,558);
  const uint32_t total=conv[i]+gather[i]+PioneerQ38ResidentOutputDelta(i,s[0],s[1],s[2],s[3])+md;
  if(uint32_t(after[i]-before[i])!=total || uint32_t(a[24+i]-a[16+i])!=md
   || uint32_t(after[i]-a[24+i])!=PioneerQ38ResidentOutputDelta(i,postReads,0,postReads,0))return false;
 }
 if(a[18]!=a[22] || a[26]!=a[30])return false;
 const uint32_t last[]={0xffb20228,0,PioneerQ38ConvPeer(119),0x75008,0,129,0,0,4,0};
 for(unsigned i=0;i<10;i++)if(command[i]!=last[i])return false;
 return true;
}
static inline bool PioneerQ38ResidentOutputPeerReceipt(const uint32_t* m,const uint32_t* q,unsigned w,
 const uint32_t* before,const uint32_t* after,const uint32_t* command,const uint32_t* p,const uint32_t* matrixBefore,
 const uint32_t* first,const uint32_t* recurrent,const uint32_t* gated,const uint32_t* clock){
 // The caller owes the SAME immutable validated canonical m/q, not copies
 // supplied by an application. This checks ONE complete counter lineage
 // from the independent pre-boot snapshot, not only the last matrix delta.
 if(!m || !q || !w || w>=120 || !before || !after || !command || !p || !matrixBefore
  || before[2]!=before[6] || after[2]!=after[6])return false;
 const auto* part=q+TT_Q38_OUTPUT_COHORT_PART+(w-1)*8;const unsigned groups=part[2],sequence=PioneerQ38ResidentOutputSequence(m,w);
 if(!groups || groups>16 || sequence==UINT32_MAX || sequence>q[7] || part[1]>=7
  || part[4]!=PioneerQ38ProjectionRoute(part[1]) || part[5]!=PioneerQ38CohortSource(m,w)[0])return false;
 uint32_t tracked[8];const unsigned n=PioneerQ38ConvBatchCount(w);
 const unsigned conv[]={3*n,8*n,5*n,3*n,2*n,4*n,5*n,2*n};
 for(unsigned i=0;i<8;i++)tracked[i]=before[i]+conv[i];
 if(w<=96){
  const auto* in=PioneerQ38CohortGdn(m,w);const auto* state=in+64;const auto* d=state+64;
  if(!first || !recurrent || !gated || !clock
   || !PioneerQ38GdnInputSummary(first,in,tracked,first+16)
   || !PioneerQ38ResidentGdnSummary(recurrent,state,first+16,recurrent+16)
   || !PioneerQ38GdnRendezvousSummary(gated,d,in,state,recurrent+16,gated+16)
   || !PioneerQ38GdnRendezvousClockReceipt(clock,d,gated[5]))return false;
  for(unsigned i=0;i<8;i++)tracked[i]=gated[16+i];
 }
 if(w<=24){const unsigned delta[]={1,16,3,1,2,10,3,2};for(unsigned i=0;i<8;i++)tracked[i]+=delta[i];}
 const unsigned reads=groups*192+2,flits=groups*1728+204;
 for(unsigned i=0;i<8;i++)if(p[i]!=tracked[i] || matrixBefore[i]!=tracked[i] || p[8+i]!=after[i]
  || uint32_t(after[i]-tracked[i])!=PioneerQ38ResidentOutputDelta(i,reads,0,flits,0))return false;
 if(p[16]!=groups*384 || p[17]!=sequence || p[18]!=w || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER
  || p[30]!=PioneerQ38CohortSource(m,w)[0] || p[31]!=PioneerQ38ConvPeer(w))return false;
 const uint32_t last[]={part[0]+(groups*192-1)*576,0,part[4],0x90000,0,PioneerQ38ConvPeer(w),0,0,576,0};
 for(unsigned i=0;i<10;i++)if(p[20+i]!=last[i] || command[i]!=last[i])return false;
 for(unsigned i=0;i<16;i++)if(p[32+i]!=PioneerQ38CohortSource(m,w)[i])return false;
 for(unsigned i=48;i<64;i++)if(p[i])return false;
 return true;
}
#endif
