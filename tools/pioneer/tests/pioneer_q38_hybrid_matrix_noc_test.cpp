/* SPDX-License-Identifier: MIT */
// Actual linked ROOT packet-transfer instructions exchange jobs and reads
// with one actual QGW7/QDM2 worker. The transport, input and ROOT private
// frame are synthetic; this is neither full Launch nor hardware execution.
#define PIONEER_Q38_HYBRID_MATRIX_TRANSITION_MACHINE_ONLY
#include "pioneer_q38_hybrid_matrix_attention_transition_test.cpp"
#define PIONEER_Q38_HYBRID_ACTUAL_ROOT_MACHINE_ONLY
#include "pioneer_q38_hybrid_actual_root_test.cpp"
#include "../PioneerQ38DensePartitionRootLayout.h"
#include <sys/wait.h>
#include <unistd.h>

namespace DR=PioneerQ38DensePartitionRootLayout;
struct PacketRoot:ActualRoot {
 TransitionImage& peer;unsigned command[10]={},counters[8]={},sources[16]={};
 unsigned packets=0,reads=0,writes=0,delivered=0,corruptedPacket=0;
 bool corruptReadback=false,corruptSummary=false,corruptCounter=false,resetRead=false;
 static constexpr unsigned Private=0x6e000,Frame=Private+768,Tracked=0x6e600,Ordinal=72;
 PacketRoot(const char* image,TransitionImage& worker):ActualRoot(image),peer(worker){
  assert(transfers.size()==2 && denseJob.size()==1 && snapshots.size()==1 && peer.boot[3]==Ordinal+1);
  sources[0]=0x16005;for(unsigned i=1;i<16;i++)sources[i]=0x12340000+i;
  for(unsigned i=0;i<64;i++)memory[Private+i*4]=memory[0x75300+i*4]=0;
  for(unsigned i=0;i<128;i++)memory[Private+256+i*4]=memory[0x75000+i*4]=0;
  for(unsigned i=0;i<64;i++)memory[Frame+i*4]=memory[DR::Header+i*4]=0;
  for(unsigned i=0;i<16;i++)memory[Private+(16+i)*4]=memory[0x75300+(16+i)*4]=
   memory[Frame+(16+i)*4]=memory[DR::Header+(16+i)*4]=sources[i];
  for(unsigned i=0;i<8;i++)memory[Private+(8+i)*4]=memory[0x75300+(8+i)*4]=
   memory[Frame+(32+i)*4]=memory[DR::Header+(32+i)*4]=peer.boot[8+i];
  memory[Frame]=memory[DR::Header]=0x31524d51;memory[Frame+5*4]=memory[DR::Header+5*4]=129;
  memory[Frame+3*4]=memory[DR::Header+3*4]=3;
  memory[Private+256+48*4]=memory[0x75000+48*4]=memory[0x75400]=1;
  memory[Private+1024]=5;
  for(unsigned i=0;i<8;i++)memory[Tracked+i*4]=0;
  for(unsigned i=0;i<64;i++)memory[DR::Summary+i*4]=0;
  for(unsigned i=0;i<64;i++)memory[0x170000+Ordinal*256+i*4]=peer.boot[i];
  const unsigned counterIndices[]={2,3,4,5,1,8,4,10};
  for(unsigned i=0;i<8;i++)memory[0x122000+Ordinal*32+i*4]=peer.transfer.counters[counterIndices[i]];
 }
 unsigned SourceOffset(unsigned i)const{return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 unsigned CounterOffset(unsigned i)const{return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
 unsigned Load(unsigned a)override{
  if(Range(a,0xffb20000,0x2000)){
   const unsigned o=a-0xffb20000;
   if(o<40)return command[o/4]^(corruptReadback && o==0?1U:0U);
   for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return 0;
   if(o>=0x240 && o<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(o==SourceOffset(i))return sources[i];
   for(unsigned i=0;i<8;i++)if(o==CounterOffset(i))return counters[i];
   assert(!"unmodeled ROOT NoC read");
  }
  return ActualRoot::Load(a);
 }
 void Store(unsigned a,unsigned value)override{
  if(Range(a,0xffb20000,40)){command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1);Submit();return;}
  ActualRoot::Store(a,value);
 }
 unsigned PeerRead(unsigned a){
  if(TransitionImage::DenseL1(a))return peer.WordRead(a);
  return peer.Get(a);
 }
 void PeerWrite(unsigned a,unsigned value){
  if(TransitionImage::DenseL1(a))peer.WordWrite(a,value);else peer.Put(a,value);
 }
 void Submit(){
  const bool write=command[7]==0x212;assert(write || !command[7]);
  const unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(local==DR::Scratch+(remote&31) && bytes && !(bytes&3) && !command[1] && !command[4] && !command[6] && !command[9]);
  assert((write?command[2]:command[5])==129 && (write?command[5]:command[2])==peer.boot[4]);
  assert((write && ((remote==HW::Job && bytes==256) || (remote==HW::Doorbell && bytes==4)
    || (remote==Dense::Header && bytes==256) || (remote==Dense::HalfQuery && bytes==6144)
    || ((remote==Dense::NewK || remote==Dense::NewV) && bytes==512) || (remote==Dense::Parameters && bytes==64)))
   || (!write && ((remote==0xffb121b0 && bytes==4) || (remote==0xffb13140 && bytes==4)
    || (Range(remote,0xffb20000,0x2000) && bytes==4)
    || (remote==HW::Boot && bytes==256) || (remote==HW::Doorbell && bytes==4)
    || (remote==HW::Status && bytes==64) || (remote==Dense::Summary && bytes==256)
    || (remote==Dense::Job && bytes==64) || (remote==Dense::Local && bytes==256)
    || (remote==Dense::Header && bytes==256) || (remote==HW::Job && bytes==256)
    || (remote==Dense::Doorbell && bytes==4) || (remote==Dense::Parameters && bytes==64)
    || (remote==Dense::Partial && bytes==12288))));
  packets++;
  if(write){
   for(unsigned i=0;i<bytes/4;i++)PeerWrite(remote+i*4,memory.at(local+i*4));writes++;
   if(remote==HW::Doorbell){delivered++;assert(peer.FinishCombined(peer.Get(HW::Doorbell)));}
  }else{
   if(remote==0xffb121b0){memory[local]=0x46800;resetRead=true;}
   else if(remote==0xffb13140){assert(resetRead);memory[local]=peer.pc;resetRead=false;}
   else if(Range(remote,0xffb20000,0x2000)){
    const unsigned offset=remote-0xffb20000;bool found=false;
    for(unsigned i=0;i<16;i++)if(offset==SourceOffset(i)){memory[local]=peer.source[i];found=true;}
    const unsigned indices[]={2,3,4,5,1,8,4,10};
    for(unsigned i=0;i<8;i++)if(offset==CounterOffset(i)){
     memory[local]=peer.transfer.counters[indices[i]];
     if(corruptCounter && i==0){memory[local]^=1;corruptedPacket=packets;}
     found=true;break;
    }
    assert(found);
   }else for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=PeerRead(remote+i*4);
   if(corruptSummary && remote==Dense::Summary){memory[local+4]^=1;corruptedPacket=packets;}
   reads++;
  }
  counters[write?4:0]++;counters[write?5:1]+=(bytes+63)>>6;counters[write?7:3]++;counters[2]++;counters[6]++;
 }
 bool Transfer(unsigned remote,unsigned bytes,bool write){
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;regs[27]=Private;
  regs[10]=Frame;regs[11]=Tracked;regs[12]=Ordinal;regs[13]=remote;regs[14]=bytes;regs[15]=write;
  pc=transfers.back();steps=0;
  while(pc!=0x60004 && pc!=0x60000){Step();assert(regs[2]>=0x6e800 && regs[2]<=0x70000);}
  return pc==0x60004;
 }
 bool Snapshot(unsigned phase,unsigned sequence){
  memory[0x123000+Ordinal*4]=sequence;
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;regs[27]=Private;
  regs[10]=Frame;regs[11]=Tracked;regs[12]=Ordinal;regs[13]=phase;pc=snapshots[0];steps=0;
  while(pc!=0x60004 && pc!=0x60000){Step();assert(regs[2]>=0x6e800 && regs[2]<=0x70000);}
  return pc==0x60004;
 }
 void CopyFrame(){
  for(unsigned i=0;i<64;i++)memory[DR::Frames+Ordinal*256+i*4]=memory[DR::Shadow+Ordinal*256+i*4]=peer.WordRead(Dense::Header+i*4);
 }
 void MakeJob(unsigned phase,unsigned sequence){
  assert(Run(denseJob[0],Ordinal,phase,sequence));
  assert(memory.at(DR::Scratch)==HW::Magic && memory.at(DR::Scratch+4)==sequence);
 }
 void TransferInput(unsigned remote,unsigned bytes){
  std::vector<unsigned> expected;
  for(unsigned i=0;i<bytes/4;i++){
   const unsigned value=PeerRead(remote+i*4);expected.push_back(value);
   memory[DR::Scratch+(remote&31)+i*4]=value;PeerWrite(remote+i*4,0x7fc00001);
  }
  assert(Transfer(remote,bytes,true));
  for(unsigned i=0;i<bytes/4;i++)assert(PeerRead(remote+i*4)==expected[i]);
 }
};
int main(int argc,char** argv){
 assert(argc==4);auto peer=std::make_unique<TransitionImage>(argv[2],argv[3]);peer->Boot();
 for(unsigned layer=0;layer<3;layer++){
  if(layer)peer->RearmTestAccounting();
  peer->Stage(layer,layer+1,4);assert(peer->FinishCombined(layer+1));
 }
 auto root=std::make_unique<PacketRoot>(argv[1],*peer);
 assert(root->Transfer(HW::Boot,256,false));assert(root->memory.at(DR::Scratch)==HW::Magic);
 for(unsigned phase=1;phase<=3;phase++){
  const unsigned sequence=3+phase;peer->StageDense(phase,sequence,false);root->CopyFrame();
  if(phase==1){
   root->TransferInput(Dense::Header,256);root->TransferInput(Dense::HalfQuery,6144);
   root->TransferInput(Dense::NewK,512);root->TransferInput(Dense::NewV,512);
  }else{
   for(unsigned i=0;i<16;i++)root->memory[DR::Parameters+3*64+i*4]=peer->WordRead(Dense::Parameters+i*4);
   root->TransferInput(Dense::Parameters,64);
  }
  root->MakeJob(phase,sequence);
  assert(root->Transfer(HW::Job,256,true));root->memory[DR::Scratch]=sequence;
  assert(root->Transfer(HW::Doorbell,4,true));
  assert(root->Transfer(HW::Doorbell,4,false));assert(root->memory.at(DR::Scratch)==sequence);
  assert(root->Transfer(HW::Status,64,false));assert(root->memory.at(DR::Scratch+4)==sequence);
  assert(root->Transfer(Dense::Job,64,false));assert(root->memory.at(DR::Scratch)==Dense::Magic);
  assert(root->Transfer(Dense::Summary,256,false));assert(root->memory.at(DR::Scratch)==Dense::Magic);
  assert(root->Transfer(Dense::Local,256,false));assert(root->memory.at(DR::Scratch)==phase);
  assert(root->Snapshot(phase,sequence));
 }
 assert(root->Transfer(Dense::Partial,12288,false));
 assert(root->packets>29 && root->writes==12 && root->delivered==3);
 for(unsigned h=0;h<6;h++)for(unsigned d=0;d<256;d++){
  const float expected=float(int(d%17)-8)*.0625f;
  const float high=PioneerAttentionFloat(root->memory.at(DR::Scratch+(h*512+d*2)*4));
  const float low=PioneerAttentionFloat(root->memory.at(DR::Scratch+(h*512+d*2+1)*4));
  assert(high+low==expected);
 }
 // Run a late corrupted-receipt branch in a copy of the already executed
 // simulated world; the faulted branch does not continue after the fault.
 for(unsigned fault=0;fault<2;fault++){
  fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
  if(!pid){
   root->corruptSummary=fault==0;root->corruptCounter=fault==1;
   assert(!root->Snapshot(3,6) && root->corruptedPacket && root->packets==root->corruptedPacket);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==0);
 }
 // ROOT command readback corruption must stop the real transfer before the
 // endpoint sees a packet, not manufacture successful completion.
 const unsigned sent=root->packets;root->corruptReadback=true;
 assert(!root->Transfer(HW::Status,64,false) && root->packets==sent);
 puts("PASS actual QGW7 ROOT NoC transfers and three Snapshot receipt/counter checks with one actual worker; corrupted receipt and counter refused; synthetic fabric/input, no device or TPS");
}
