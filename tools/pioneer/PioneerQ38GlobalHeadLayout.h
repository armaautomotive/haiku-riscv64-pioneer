/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_HEAD_LAYOUT_H
#define PIONEER_Q38_GLOBAL_HEAD_LAYOUT_H
#include "PioneerQ38GlobalChainLayout.h"
// Fixed512B immutable DATA; no weight/init/all64/owner/launch authority.
namespace PioneerQ38GlobalHeadLayout {
namespace C=PioneerQ38GlobalChainLayout;namespace G=PioneerQ38GlobalWorkerLayout;
enum {Words=128,Stripe=64,StripeWords=8,Jobs=973,Waves=9,RowBytes=4240};
static inline bool Shape(const uint32_t* h,const uint32_t* boot,unsigned bank,unsigned workspace){
 if(!h || !C::BootShape(boot) || bank>=7 || workspace<0x04000000 || workspace>0xc0000000 || (workspace&4095))return false;
 const unsigned fixed[]={C::HeadMagic,1,128,bank,workspace,workspace+C::HeadOffset,0,0,5120,248320,973,9,119,14,4240,20480};
 for(unsigned i=0;i<16;i++)if(i!=6 && i!=7 && h[i]!=fixed[i])return false;
 if(h[6]>=7 || h[7]<0x04000000 || h[7]>0xc3ffb000 || (h[7]&63)
  || (h[6]==bank && h[7]<workspace+C::WorkspaceBytes && workspace<h[7]+20480))return false;
 for(unsigned i=0;i<16;i++)if(h[16+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++)if(h[32+i]!=boot[8+i])return false;
 const unsigned witness[]={G::Magic,C::Magic,64,0,16,16,0,1};
 for(unsigned i=0;i<8;i++)if(h[40+i]!=witness[i])return false;
 for(unsigned i=48;i<64;i++)if(h[i])return false;
 unsigned rows=0;
 for(unsigned s=0;s<7;s++){
  const auto* p=h+Stripe+s*StripeWords;
  if(p[0]!=s || p[1]<0x04000000 || (p[1]&4095) || (p[2]&63) || p[2]>=248320
   || (p[3]!=35456 && p[3]!=35520) || p[3]>248320-p[2] || p[4]!=RowBytes || p[5]!=p[3]*RowBytes
   || p[6]!=((p[5]+4095)&~4095U) || p[1]>0xc4000000-p[6] || p[7]!=139
   || (s==bank && p[1]<workspace+C::WorkspaceBytes && workspace<p[1]+p[6])
   || (s==h[6] && p[1]<h[7]+20480 && h[7]<p[1]+p[6]))return false;
  for(unsigned t=0;t<s;t++){
   const auto* q=h+Stripe+t*StripeWords;if(p[2]<q[2]+q[3] && q[2]<p[2]+p[3])return false;
  }
  rows+=p[3];
 }
 for(unsigned i=120;i<128;i++)if(h[i])return false;
 return rows==248320; // disjoint bounded stripes cover every row exactly once
}
static inline bool Job(const uint32_t* h,unsigned ordinal,unsigned previous,const uint32_t* boot,uint32_t* j){
 if(!h || !boot || !j || ordinal>=Jobs || previous>=0x7ffffffe || !G::BootShape(boot))return false;
 using Address=__UINTPTR_TYPE__;Address o=reinterpret_cast<Address>(j),a=reinterpret_cast<Address>(h),b=reinterpret_cast<Address>(boot),max=~Address(0);
 if(((o|a|b)&3) || o>max-256 || a>max-512 || b>max-256 || (o<a+512 && a<o+256) || (o<b+256 && b<o+256))return false;
 unsigned worker=ordinal,wave=0;while(worker>=119){worker-=119;wave++;}worker++;
 if(boot[3]!=worker)return false;
 for(unsigned i=0;i<8;i++)if(boot[8+i]!=h[32+i])return false;
 unsigned stripe=ordinal,index=0;while(stripe>=7){stripe-=7;index++;}
 const auto* p=h+Stripe+stripe*StripeWords;
 if(p[0]!=stripe || p[7]!=139 || (p[3]!=35456 && p[3]!=35520) || index>=139)return false;
 const unsigned remainder=p[3]/16-15*139,done=index*15+(index<remainder?index:remainder),groups=15+unsigned(index<remainder);
 uint32_t stage[64];for(unsigned i=0;i<64;i++)stage[i]=0;
 const unsigned head[]={G::Magic,previous+1,G::GlobalMatrix,worker,G::Peer(worker),64,0,2025+wave};
 for(unsigned i=0;i<8;i++){stage[i]=head[i];stage[8+i]=boot[8+i];}
 const unsigned matrix[]={0x31483851,14,3392,0,0,5120,p[1]+done*16*RowBytes,groups,PioneerQ38ProjectionRoute(p[0]),1};
 for(unsigned i=0;i<10;i++)stage[16+i]=matrix[i];
 stage[26]=1;stage[27]=wave;stage[28]=p[2]+done*16;stage[29]=248320;stage[30]=1;stage[31]=G::JobWitness;
 if(!G::JobShape(stage,boot,previous,0))return false;
 for(unsigned i=0;i<64;i++)j[i]=stage[i];
 return true;
} // caller must separately seal the validated immutable header and canonical source
}
#endif
