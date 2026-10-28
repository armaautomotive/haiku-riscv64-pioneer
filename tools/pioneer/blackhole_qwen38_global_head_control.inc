/* SPDX-License-Identifier: MIT */
// Inside the NEW global parent namespace. No host-ready entry point. Only
// reached after all64 real returning calls under this SAME private owner.
namespace H=PioneerQ38GlobalHeadLayout;namespace G=PioneerQ38GlobalWorkerLayout;
static const uint32_t* PeerBoot(unsigned w){return reinterpret_cast<const uint32_t*>(R::PeerBoots+w*256);}
static const uint32_t* PeerJob(unsigned w){return reinterpret_cast<const uint32_t*>(R::PeerJobs+w*256);}
static void HeadSeal(const uint32_t* h,bool counters=true){
 Scope(counters);
#ifdef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
#ifdef PIONEER_Q38_TOKEN_CANCEL_BOUNDARY
 if(Owner()->phase==7){if(!Owner()->state[Completed])scalar_fault_park();}
 else
#endif
 if(Owner()->phase==5){if(Owner()->state[Completed] || Owner()->state[Layer])scalar_fault_park();}
 else if((Owner()->phase!=4 && Owner()->phase!=6) || Owner()->state[Completed]!=64 || Owner()->state[Layer]!=63)scalar_fault_park();
#else
 if(Owner()->phase!=4 || Owner()->state[Completed]!=64 || Owner()->state[Layer]!=63)scalar_fault_park();
#endif
 for(unsigned i=0;i<128;i++)if(Words(L::Head)[i]!=h[i])scalar_fault_park();
}
static void Packet(const uint32_t* h,unsigned route,unsigned remote,unsigned bytes,bool write){
 if(!bytes || bytes>16384-(remote&31) || ((bytes|remote)&3))scalar_fault_park();
 HeadSeal(h);auto* p=Owner();unsigned local=R::Scratch+(remote&31);uint32_t expected[8];
 for(unsigned i=0;i<8;i++)expected[i]=p->state[Counts+i]+(i==(write?5U:1U)?(bytes+63)>>6:i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U);
 const unsigned command[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];if(done)break;
 }
 if(!done)scalar_fault_park();
 HeadSeal(h,false);
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)p->state[Counts+i]=expected[i];
}
static bool Csr(unsigned a){
 if(a==0xffb121b0 || a==0xffb13140)return true;
 for(unsigned i=0;i<16;i++)if(a==0xffb20000+Source(i))return true;
 for(unsigned i=0;i<8;i++)if(a==0xffb20000+Counter(i))return true;
 return false;
}
static void PeerPacket(const uint32_t* h,unsigned w,unsigned remote,unsigned bytes,bool write){
 if(w>=119 || (write?!((remote==G::Job && bytes==256) || (remote==G::Doorbell && bytes==4)):
  !((remote==G::Boot && bytes==256) || (remote==G::Job && bytes==256) || (remote==G::Status && bytes==64)
  || (remote==G::Doorbell && bytes==4) || (remote==G::Partial && bytes==512) || (Csr(remote) && bytes==4))))scalar_fault_park();
 Packet(h,G::Peer(w+1),remote,bytes,write);
}
static unsigned Register(const uint32_t* h,unsigned w,unsigned address){PeerPacket(h,w,address,4,false);return *Words(R::Scratch+(address&31));}
static void BankRead(const uint32_t* h,unsigned bank,unsigned address,unsigned local,unsigned bytes){
 const unsigned workspace=Owner()->state[Workspace],selected=Owner()->state[Bank];
 const bool hidden=bank==selected && address>=workspace+R::FinalOffset && address<=workspace+R::FinalOffset+20480 && bytes<=workspace+R::FinalOffset+20480-address;
 const bool norm=bank==h[6] && address>=h[7] && address<=h[7]+20480 && bytes<=h[7]+20480-address;
 if((!hidden && !norm) || !bytes || ((address|local|bytes)&63) || bytes>16384)scalar_fault_park();
 Packet(h,PioneerQ38ProjectionRoute(bank),address,bytes,false);
 for(unsigned i=0;i<bytes/4;i++)Words(local)[i]=Words(R::Scratch)[i];
}
static void InputSeal(){
 for(unsigned i=0;i<5120;i++)if(Words(R::Original)[i]!=Words(R::OriginalCopy)[i] || Words(R::Input)[i]!=Words(R::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<2560;i++)if(Words(R::Native)[i]!=Words(R::NativeShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<32;i++)if(Words(R::Scales)[i]!=Words(R::ScaleShadow)[i])scalar_fault_park();
}
static void Normalize(const uint32_t* h){
 unsigned workspace=Owner()->state[Workspace],bank=Owner()->state[Bank];
 for(unsigned at=0;at<20480;at+=16384){
  unsigned bytes=20480-at;if(bytes>16384)bytes=16384;
  BankRead(h,bank,workspace+R::FinalOffset+at,R::Original+at,bytes);
  BankRead(h,h[6],h[7]+at,R::Weight+at,bytes);
 }
 for(unsigned i=0;i<5120;i++)Words(R::OriginalCopy)[i]=Words(R::Original)[i];
 HeadSeal(h);if(!q38_global_chain_learned_rms(reinterpret_cast<const float*>(R::Original),reinterpret_cast<const float*>(R::Weight),reinterpret_cast<float*>(R::Input)))scalar_fault_park();HeadSeal(h);
 for(unsigned i=0;i<5120;i++)Words(R::InputShadow)[i]=Words(R::Input)[i];
 for(unsigned i=0;i<32;i++)Words(R::Scales)[i]=0;
 for(unsigned at=0;at<5120;at+=256){
  // Same existing576B numeric capsule,not a second quantizer or CPU path.
  if(!q38_global_chain_quantize14(reinterpret_cast<const float*>(R::Input+at*4),reinterpret_cast<void*>(R::QuantizedChunk)))scalar_fault_park();
  for(unsigned i=0;i<128;i++)Words(R::Native)[at/2+i]=Words(R::QuantizedChunk)[i];
  Words(R::Scales)[at>>8]=Words(R::QuantizedChunk)[128];
 }
 for(unsigned i=0;i<2560;i++)Words(R::NativeShadow)[i]=Words(R::Native)[i];
 for(unsigned i=0;i<32;i++)Words(R::ScaleShadow)[i]=Words(R::Scales)[i];
 Fence();InputSeal();
}
static void Wait(const uint32_t* h,unsigned w,bool stopped){
 unsigned n=Words(R::Nonces)[w];const auto* j=PeerJob(w);
 if(!n || !G::JobShape(j,PeerBoot(w),n-1,0) || j[1]!=n)scalar_fault_park();
 for(unsigned poll=0;poll<65536;poll++){
  if(Register(h,w,0xffb121b0)!=0x46800)scalar_fault_park();
  PeerPacket(h,w,G::Status,64,false);uint32_t s[16];for(unsigned i=0;i<16;i++)s[i]=Words(R::Scratch)[i];
  if(s[2] || s[3]!=G::Magic || s[0]<n-1 || s[0]>n || s[1]<n-1 || s[1]>s[0]
   || (s[4] && s[4]!=j[2]) || s[5] || s[6]>64
#ifdef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
   || s[7]>=8192
#else
   || s[7]
#endif
  )scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  unsigned pc=Register(h,w,0xffb13140);
  if(s[0]!=n || s[1]!=n)continue;
  if(s[4]!=(stopped?unsigned(G::Stop):0U) || s[6]!=j[5] || s[7]!=j[6])scalar_fault_park();
  if(stopped){
   if(pc!=0x20060 && pc!=0x20064)continue;
   if(Register(h,w,0xffb121b0)!=0x46800)scalar_fault_park();
   pc=Register(h,w,0xffb13140);if(pc!=0x20060 && pc!=0x20064)continue;
  }else if(pc!=0x2004c && pc!=0x20050)continue;
  return;
 }
 scalar_fault_park();
}
static void Closed(const uint32_t* h,unsigned w,bool stopped=false){
 Wait(h,w,stopped);const auto* b=PeerBoot(w);
 if(!G::BootShape(b) || b[3]!=w+1 || b[4]!=G::Peer(w+1))scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(b[8+i]!=Owner()->boot[8+i])scalar_fault_park();
 PeerPacket(h,w,G::Boot,256,false);for(unsigned i=0;i<64;i++)if(Words(R::Scratch)[i]!=b[i])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(Register(h,w,0xffb20000+Source(i))!=b[16+i])scalar_fault_park();
 PeerPacket(h,w,G::Job,256,false);for(unsigned i=0;i<64;i++)if(Words(R::Scratch)[i]!=PeerJob(w)[i])scalar_fault_park();
 if(Register(h,w,G::Doorbell)!=Words(R::Nonces)[w])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(Register(h,w,0xffb20000+Counter(i))!=Words(R::Counters)[w*8+i])scalar_fault_park();
}
static void Dispatch(const uint32_t* h,unsigned w){
 for(unsigned i=0;i<64;i++)Words(R::Scratch)[i]=PeerJob(w)[i];
 PeerPacket(h,w,G::Job,256,true);
}
static void Release(const uint32_t* h,unsigned w){
 unsigned n=Words(R::Nonces)[w];if(n>=0x7ffffffe || PeerJob(w)[1]!=n+1)scalar_fault_park();
 *Words(R::Scratch)=n+1;PeerPacket(h,w,G::Doorbell,4,true);Words(R::Nonces)[w]=n+1;
}
static void Gather(const uint32_t* h,unsigned w,unsigned ordinal,unsigned& bestRow,unsigned& bestBits,unsigned& rows){
 uint32_t expectedJob[64];unsigned n=Words(R::Nonces)[w];
#ifdef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
 if(!n || !T::Job(h,Owner()->state[Position],ordinal,n-1,PeerBoot(w),expectedJob))scalar_fault_park();
#else
 if(!n || !H::Job(h,ordinal,n-1,PeerBoot(w),expectedJob))scalar_fault_park();
#endif
 for(unsigned i=0;i<64;i++)if(PeerJob(w)[i]!=expectedJob[i])scalar_fault_park();
 Wait(h,w,false);const auto* j=PeerJob(w);const auto* b=PeerBoot(w);
 PeerPacket(h,w,G::Partial,512,false);uint32_t p[64];for(unsigned i=0;i<64;i++)p[i]=Words(R::Scratch)[i];
 const auto* proof=Words(R::Scratch+256);const unsigned groups=j[23],reads=groups*20+2,flits=groups*1060+162;
 const unsigned expected[]={G::PartialMagic,j[1],64,
#ifdef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
  j[6],
#else
  0,
#endif
  w+1,j[27],j[28],groups*16,p[8],p[9],1,j[7],248320,5120,14,1};
 for(unsigned i=0;i<16;i++)if(p[i]!=expected[i])scalar_fault_park();
 if((p[9]&0x7f800000)==0x7f800000 || p[8]<j[28] || p[8]>=j[28]+groups*16 || j[2]!=G::GlobalMatrix)scalar_fault_park();
 if(proof[16]!=groups*320 || proof[17]!=j[1] || proof[18]!=w+1 || proof[19]!=0x32504651 || proof[30]!=b[16] || proof[31]!=b[4])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(p[34+i]!=b[16+i] || proof[32+i]!=b[16+i])scalar_fault_park();
 for(unsigned i=48;i<64;i++)if(proof[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(p[50+i]!=b[8+i])scalar_fault_park();
 for(unsigned i=58;i<64;i++)if(p[i])scalar_fault_park();
 const unsigned command[]={j[22]+(groups*20-1)*3392,0,j[24],0x90000,0,b[4],0,0,3392,0};
 for(unsigned i=0;i<10;i++)if(p[24+i]!=command[i] || proof[20+i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
  if(proof[i]!=Words(R::Counters)[w*8+i] || uint32_t(proof[8+i]-proof[i])!=delta || p[16+i]!=proof[8+i])scalar_fault_park();
 }
 if(p[18]!=p[22])scalar_fault_park();
 for(unsigned i=0;i<8;i++)Words(R::Counters)[w*8+i]=p[16+i];
 Closed(h,w);
 if(bestRow==248320 || G::Better(p[9],p[8],bestBits,bestRow)){bestRow=p[8];bestBits=p[9];}
 rows+=groups*16;
}
#ifdef PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK
#include "blackhole_qwen38_global_feedback_control.inc"
#endif
#ifdef PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE
#include "blackhole_qwen38_token_head_control.inc"
#else
static __attribute__((noreturn,noinline)) void RunHead(){
 uint32_t header[128];auto* owner=Owner();unsigned workspace=owner->state[Workspace],bank=owner->state[Bank];
 Transfer(workspace+L::HeadOffset,512,false);
 for(unsigned i=0;i<128;i++)header[i]=Words(L::Head)[i]=Words(R::Scratch)[i];
 if(!H::Shape(header,owner->boot,bank,workspace))scalar_fault_park();
 HeadSeal(header);Normalize(header);
 unsigned accepted=0,rows=0,bestRow=248320,bestBits=0;
 for(unsigned w=0;w<119;w++)Closed(header,w);
 for(unsigned wave=0;wave<9;wave++){
  InputSeal();unsigned workers=wave==8?21:119;Words(R::Status)[5]=0x200+wave;
  for(unsigned w=0;w<workers;w++){
   uint32_t job[64];if(!H::Job(header,wave*119+w,Words(R::Nonces)[w],PeerBoot(w),job))scalar_fault_park();
   for(unsigned i=0;i<64;i++)Words(R::PeerJobs+w*256)[i]=job[i];
   Dispatch(header,w);
  }
  for(unsigned w=0;w<workers;w++)Release(header,w);
  for(unsigned w=0;w<workers;w++){Gather(header,w,wave*119+w,bestRow,bestBits,rows);accepted++;}
  InputSeal();HeadSeal(header);
 }
 if(accepted!=973 || rows!=248320 || bestRow>=248320)scalar_fault_park();
 for(unsigned w=0;w<119;w++)Closed(header,w);
#ifdef PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK
 uint32_t feedback[16];Feedback(header,bestRow,feedback);
#endif
 // A complete output result is not a token-state commit. Stop ALL real
 // workers and publish only this NEW one-pass diagnostic candidate record.
 for(unsigned w=0;w<119;w++){
  unsigned n=Words(R::Nonces)[w];if(n>=0x7ffffffe)scalar_fault_park();
  uint32_t j[64];for(unsigned i=0;i<64;i++)j[i]=0;
  const unsigned h[]={G::Magic,n+1,G::Stop,w+1,G::Peer(w+1),64,0,9999};
  for(unsigned i=0;i<8;i++){j[i]=h[i];j[8+i]=owner->boot[8+i];}
  if(!G::JobShape(j,PeerBoot(w),n,0))scalar_fault_park();
  for(unsigned i=0;i<64;i++)Words(R::PeerJobs+w*256)[i]=j[i];
  Dispatch(header,w);
 }
 for(unsigned w=0;w<119;w++)Release(header,w);
 for(unsigned w=0;w<119;w++)Closed(header,w,true);
 InputSeal();HeadSeal(header);
#ifdef PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK
 PublishFeedback(header,feedback);
#endif
 for(unsigned i=0;i<64;i++)Words(L::Summary)[i]=0;
 const unsigned summary[]={0,1,64,973,248320,bestRow,bestBits,0,119,9,0,1};
 for(unsigned i=0;i<12;i++)Words(L::Summary)[i]=summary[i];
 for(unsigned i=0;i<8;i++){Words(L::Summary)[12+i]=owner->boot[8+i];Words(L::Summary)[20+i]=owner->state[Counts+i];}
 for(unsigned i=0;i<16;i++)Words(L::Summary)[28+i]=owner->boot[16+i];
 Fence();Words(L::Summary)[0]=L::CandidateMagic;Fence();q38_global_chain_stopped();
}
#endif
