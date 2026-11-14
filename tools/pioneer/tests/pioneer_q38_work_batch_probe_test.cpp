/* SPDX-License-Identifier: MIT */
// Exercise actual fixed DEBUG client against inert synthetic POSIX/PCI replies.
#define open batch_fake_open
#define close batch_fake_close
#define ioctl batch_fake_ioctl
#define PIONEER_Q38_BATCH_PROBE_MAIN batch_probe_main
#include "../pioneer_q38_work_batch_probe.cpp"
#undef PIONEER_Q38_BATCH_PROBE_MAIN
#undef ioctl
#undef close
#undef open
#include <cassert>
#include <cstdarg>

static unsigned fault=0,opens=0,closes=0,begins=0,ends=0,batches=0,uploads=0;
static std::vector<uint8_t> bank[2];
int batch_fake_open(const char*,int,...){opens++;return 73;}
int batch_fake_close(int fd){assert(fd==73);closes++;return 0;}
int batch_fake_ioctl(int fd,unsigned long op,...){
 assert(fd==73);va_list args;va_start(args,op);void* data=va_arg(args,void*);const size_t bytes=va_arg(args,size_t);va_end(args);
 if(op==TT_PCI_PROBE_RELOAD_STATUS){
  auto& id=*static_cast<tt_pioneer_driver_reload*>(data);assert(bytes==sizeof(id));
  id.driver=55;id.build_tag=fault==1?0x2026102100010012ULL:0x2026111300010152ULL;id.generation=9;id.guard_version=1;return 0;
 }
 if(op==TT_PCI_PROBE_Q38_WORK_BATCH_SUPPORT){
  auto& s=*static_cast<PioneerQ38WorkBatchSupport*>(data);s={1,2,55,0,0x2026111300010152ULL};return 0;
 }
 if(op==TT_PCI_PROBE_Q38_WORK_SUPPORT){
  auto& s=*static_cast<tt_q38_dispatch_support*>(data);s={55,TT_Q38_WORK_SUPPORT_VERSION,119,256,17408,7,64,1,0};return 0;
 }
 if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT){
  auto& s=*static_cast<tt_q38_bank_support*>(data);s={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};return 0;
 }
 if(op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END){
  auto& r=static_cast<tt_q38_bank_session*>(data)->result;r={};r.version=1;
  if(op==TT_PCI_PROBE_Q38_BANK_BEGIN){begins++;if(fault==2)return -1;r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;}
  else {ends++;if(fault==6)return -1;r.state=TT_RESIDENT_SESSION_CLOSED;r.pci_restored=r.host_released=1;}
  return 0;
 }
 if(op==TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD){
  auto& b=*static_cast<tt_q38_bank_bulk*>(data);uploads++;if(fault==3)return -1;assert(b.bank<2);
  const size_t end=size_t(b.offset)+b.pages*4096;if(bank[b.bank].size()<end)bank[b.bank].resize(end);
  memcpy(bank[b.bank].data()+b.offset,b.data,b.pages*4096);
  b.result={};b.result.pages=b.pages;b.result.host_words=b.pages*1024;b.result.host_ready=b.result.l1_prepared=1;
  auto& t=b.result.transfer;t.version=1;t.state=TT_STORE_PASSED;t.requests=t.completed=6;
  t.wrote_device=t.host_write_issued=t.host_visible=t.readback_complete=1;
  for(unsigned s=0;s<6;s++)for(unsigned c=0;c<4;c++){t.before[s][c]=1;t.after[s][c]=1+(c==1?b.pages*64:1);}
  return 0;
 }
 if(op==TT_PCI_PROBE_Q38_WORK_BATCH_RUN){
  auto& p=*static_cast<PioneerQ38WorkBatchPacket*>(data);batches++;if(fault==4)return -1;
  for(unsigned i=0;i<2;i++){
   auto& j=p.jobs[i];auto& r=j.result;r={};r.version=1;r.state=TT_RUNTIME_PASSED;r.peers=j.peers;r.nonce=i+1;
   r.root_started=r.root_completed=r.root_held=1;r.root_marker=0x314d3851;r.root_count=j.peers;
   for(unsigned w=0;w<j.peers;w++){
    const unsigned size=(j.inputs/PioneerQ38CohortBlockInputs(j.type))*PioneerQ38PanelTileBytes(j.type);
    assert(bank[w].size()>=64+size);std::vector<uint8_t> packed(bank[w].begin()+64,bank[w].begin()+64+size);
    float result[256]={};assert(Reference(j,packed,w,result));
    for(unsigned row=0;row<256;row++)j.output[w][row]=PioneerAttentionBits(result[row]);
   }
  }
  if(fault==5)p.jobs[1].output[0][0]^=1;
  if(fault==7)p.jobs[1].result.nonce++;
  if(fault==8)p.jobs[1].activation[0]^=1;
  return 0;
 }
 assert(false);return -1;
}
int main(){
 char name[]="probe",option[]="--fixed-synthetic";char* args[]={name,option,nullptr};
 for(fault=0;fault<9;fault++){
  opens=closes=begins=ends=batches=uploads=0;for(auto& b:bank)b.clear();
  const int result=batch_probe_main(2,args);
  assert(opens==1 && closes==1 && result==(fault?1:0));
  if(fault==1)assert(!begins && !uploads && !batches && !ends);
  if(fault==2)assert(begins==1 && !uploads && !batches && !ends);
  if(fault==3)assert(begins==1 && uploads==1 && !batches && !ends);
  if(fault==4 || fault==5 || fault==7 || fault==8)assert(begins==1 && batches==1 && !ends);
  if(fault==0 || fault==6)assert(begins==1 && batches==1 && ends==1);
 }
 puts("PASS actual fixed batch DEBUG probe simulated transport/refusal/lost-reply/numeric/END faults; NO DEVICE");
}
