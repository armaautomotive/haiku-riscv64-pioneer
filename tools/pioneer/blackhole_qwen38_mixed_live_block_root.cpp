/* SPDX-License-Identifier: MIT */
// Persistent device-input block lifecycle. Native ownership/journals are
// separately mandatory; this code/geometry is NEVER a launch capability.
#include "blackhole_qwen38_mixed_block_root.cpp"
#ifdef PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND
#include "blackhole_qwen38_token_kv_append.inc"
#endif
extern "C" unsigned mixed_live_block_wait(unsigned);
extern "C" void mixed_live_block_stopped() __attribute__((noreturn));
namespace LiveBlock {
using namespace DensePartitionRoot;
static void Input(L::PrivateSeal* p){
 const auto* o=p->outer;uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 Joined::Read(o[74],PioneerQ38ProjectionRoute(o[73]),L::Original,16384,tracked);
 Joined::Read(o[74]+16384,PioneerQ38ProjectionRoute(o[73]),L::Original+16384,4096,tracked);
 for(unsigned i=0;i<5120;i++){
  float value=Floats(L::Original)[i];if(!PioneerQ38StateDomain(value))scalar_fault_park();Words(L::OriginalCopy)[i]=PioneerAttentionBits(value);
 }
 q38_mixed_root_seal();
}
static void Compute(L::PrivateSeal* p){
 Block::Norm(p->outer,0);Joined::Capture(p->outer);Copy(L::Input,L::RawCopy,5120);
 for(unsigned slot=0;slot<3;slot++){Joined::BindProjection(p,slot);if(Projection::Run(p->frame)!=(slot?1024U:12288U))scalar_fault_park();}
 Joined::Prepare(p);Joined::BindDense(p);if(q38_joined_dense_frame()!=6144)scalar_fault_park();
#ifdef PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND
 // The attention component consumes the current row directly from L1; it
 // deliberately does NOT own canonical cache publication. Append/read back
 // here, before the following projection/FFN reuses the prepared-row L1.
 Joined::AppendTokenKv(p);
#endif
 Joined::BindProjection(p,3);if(Projection::Run(p->frame)!=5120)scalar_fault_park();
 p->phase=7;Words(L::Status)[5]=7;Block::Norm(p->outer,1);Block::CaptureFfn(p->outer);
 for(unsigned slot=0;slot<2;slot++){Block::BindFfn(p,slot);if(Projection::Run(p->frame)!=17408)scalar_fault_park();}
 Block::Activation(p);Block::BindFfn(p,2);if(Projection::Run(p->frame)!=5120)scalar_fault_park();
 p->phase=12;Words(L::Status)[5]=12;
 for(unsigned i=0;i<5120;i++){
  float a=Floats(L::Projected)[i],b=Floats(L::PostResidual)[i],value=a+b;
  if(!PioneerQ38StateDomain(a) || !PioneerQ38StateDomain(b) || !PioneerQ38StateDomain(value))scalar_fault_park();
  Words(L::Final)[i]=PioneerAttentionBits(value);
 }
 Block::FfnSeal();Block::WriteFinal(p->outer);Block::FfnSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
}
static void StopJob(const uint32_t* d,unsigned w,unsigned n){
 for(unsigned i=0;i<64;i++)Words(L::Scratch)[i]=0;
 const unsigned head[]={M::Magic,n,unsigned(M::Stop),w+1,M::Peer(w+1),d[42],d[40],d[6]+1};
 for(unsigned i=0;i<8;i++){Words(L::Scratch)[i]=head[i];Words(L::Scratch)[8+i]=d[32+i];}
}
static void Stopped(const uint32_t* d,uint32_t* tracked,unsigned w){
 const unsigned n=Words(L::Nonces)[w];bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  Transfer(d,tracked,w,M::Status,64,false);uint32_t s[16];for(unsigned i=0;i<16;i++)s[i]=Words(L::Scratch)[i];
  if(s[2] || s[3]!=M::Magic || s[0]<n-1 || s[0]>n || s[1]<n-1 || s[1]>s[0] || (s[4] && s[4]!=M::Stop) || s[5]
   || s[6]!=d[42] || s[7]!=d[40])scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  if(s[0]!=n || s[1]!=n || s[4]!=M::Stop)continue;
  unsigned a=Register(d,tracked,w,0xffb13140);
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  unsigned b=Register(d,tracked,w,0xffb13140);
  // Debug PCs are speculative snapshots. No snapshot alone is completion
  // or a negative execution proof;require the real STOP record as well.
  if((a!=0x20060 && a!=0x20064) || (b!=0x20060 && b!=0x20064))continue;
  done=true;break;
 }
 if(!done)scalar_fault_park();
 BootSeal(d,tracked,w);Transfer(d,tracked,w,M::Job,256,false);
 for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=Words(L::PeerJobs+w*256)[i])scalar_fault_park();
 if(Register(d,tracked,w,M::Doorbell)!=n)scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(L::Counters)[w*8+i])scalar_fault_park();
}
static void Stop(L::PrivateSeal* p,unsigned previous){
 p->phase=13;Words(L::Status)[0]=p->outer[48];Words(L::Status)[4]=2;Words(L::Status)[5]=13;
 const auto* d=p->frame;Block::FfnSeal();q38_mixed_root_seal();uint32_t tracked[8];
 for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 // Audit ALL119 existing closed chains before the FIRST STOP write.
 for(unsigned w=0;w<119;w++){
  if(Words(L::Nonces)[w]>=0x7ffffffe)scalar_fault_park();
  Wait(d,tracked,w,0);BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
  for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(L::Counters)[w*8+i])scalar_fault_park();
 }
 for(unsigned w=0;w<119;w++){
  StopJob(d,w,Words(L::Nonces)[w]+1);Copy(L::PeerJobs+w*256,L::Scratch,64);Transfer(d,tracked,w,M::Job,256,true);
 }
 for(unsigned w=0;w<119;w++){
  *Words(L::Scratch)=Words(L::Nonces)[w]+1;Transfer(d,tracked,w,M::Doorbell,4,true);Words(L::Nonces)[w]++;
 }
 for(unsigned w=0;w<119;w++)Stopped(d,tracked,w);
 Block::FfnSeal();q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
 for(unsigned i=0;i<64;i++)Words(L::StopSummary)[i]=0;
 const unsigned head[]={0,p->outer[48],previous,119,d[42],d[40],129,1};
 for(unsigned i=0;i<8;i++){Words(L::StopSummary)[i]=head[i];Words(L::StopSummary)[8+i]=tracked[i];Words(L::StopSummary)[26+i]=d[32+i];}
 for(unsigned i=0;i<10;i++)Words(L::StopSummary)[16+i]=Words(0xffb20000)[i];
 for(unsigned i=0;i<16;i++)Words(L::StopSummary)[34+i]=p->boot[16+i];
 Fence();Words(L::StopSummary)[0]=L::StopMagic;Fence();Words(L::Status)[1]=p->outer[48];Fence();mixed_live_block_stopped();
}
}
extern "C" void mixed_live_block_main(){
 using namespace DensePartitionRoot;L::PrivateSeal seal;uint32_t counters[8];
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(L::Boot)[i];
 if(!L::BootShape(seal.boot) || *Words(L::Doorbell) || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=seal.boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
 if(counters[2]!=counters[6])scalar_fault_park();
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 for(unsigned i=0;i<16;i++)Words(L::Status)[i]=0;
 for(unsigned i=0;i<64;i++)Words(L::StopSummary)[i]=0;
 Words(L::Status)[3]=L::Magic;Fence();unsigned previous=0;
 for(;;){
  unsigned n=mixed_live_block_wait(previous);for(unsigned i=0;i<128;i++)seal.outer[i]=Words(L::Outer)[i];
  const auto* o=seal.outer;bool stop=o[72]==L::Stop;
  if(n!=previous+1 || n!=o[48] || (stop?!previous || !L::StopShape(o):!L::RunShape(o)))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(o[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(o[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  if(stop)LiveBlock::Stop(&seal,previous);
  for(unsigned i=0;i<64;i++){Words(L::JointSummary)[i]=0;seal.frame[i]=Words(L::Header)[i]=0;}
  seal.phase=1;Words(L::Status)[0]=n;Words(L::Status)[4]=1;Words(L::Status)[5]=1;
  LiveBlock::Input(&seal);LiveBlock::Compute(&seal);
  for(unsigned i=0;i<8;i++){counters[i]=*Words(0xffb20000+Counter(i));Words(L::JointSummary)[8+i]=counters[i];}
  const unsigned result[]={n,o[3],o[4],5120,119,7,17408};for(unsigned i=0;i<7;i++)Words(L::JointSummary)[1+i]=result[i];
  Fence();Words(L::JointSummary)[0]=L::Magic;Fence();previous=n;
  Words(L::Status)[4]=0;Words(L::Status)[5]=12;Words(L::Status)[6]=o[3];Words(L::Status)[7]=o[4];Fence();Words(L::Status)[1]=n;Fence();
 }
}
