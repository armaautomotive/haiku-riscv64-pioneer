/* SPDX-License-Identifier: MIT */
// Test only: one genuine Qwen FFN-down shard, original weight bytes and
// fixed independent synthetic activations. No Work mode or model inference.
#include <Drivers.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <memory>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "PioneerQwen38Gguf.h"
#include "PioneerQwen38Panel.h"
#include "PioneerQ38TwoTokenQ4NativeProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"

namespace N=PioneerQ38TwoTokenQ4Native;
namespace L=N::Q::L;
constexpr unsigned Width=17408,Blocks=Width/256,Bytes=Blocks*L::TileBytes,Pages=(Bytes+4095)/4096;
static_assert(Bytes==156672 && Pages==39,"fixed shard; never upload the whole model");

static bool Call(int fd,uint32_t op,void* data,size_t bytes)
{
 if(!ioctl(fd,op,data,bytes))return true;
 fprintf(stderr,"Q8T4_STOP operation%08x errno%d;NO_RETRY\n",op,errno);return false;
}
static void ReportCompareFailure(const N::Request& receipt)
{
 // Host copy only: no additional device open, read, reset or END.
 const auto& c=receipt.cleanup;
 fprintf(stderr,"Q8T4_COMPARE_FAILED compared%u matched%u cold%u cleanup_state%u active%u pci%u host%u cleanup_cold%u;NO_RATE\n",
  receipt.compared,receipt.matched,receipt.cold,c.state,c.active,c.pci_restored,c.host_released,c.cold);
 for(unsigned a=0;a<N::Arms;a++){
  const auto& r=receipt.arms[a];
  fprintf(stderr,"Q8T4_ARM index%u state%u prepared%u reset%u started%u completed%u stopped%u recovery%u marker%08x phase%08x panels%u polls%u\n",
   a,r.state,r.prepared,r.context_reset,r.started,r.completed,r.stopped,r.recovery_required,r.marker,r.phase,r.panels,r.polls);
 }
}
static float Float(uint32_t bits){float f;memcpy(&f,&bits,4);return f;}
static uint32_t Bits(float f){uint32_t bits;memcpy(&bits,&f,4);return bits;}
static int Value(uint16_t w){return int(w&1023)*((w&0x8000)?-1:1);}

static bool Fixture(FILE* file,N::Q::Job& job,std::vector<uint8_t>& data,uint32_t (&expected)[32])
{
 PioneerQwen38Gguf model;
 if(!model.Open(file))return false;
 const auto* t=model.Find("blk.0.ffn_down.weight");
 if(!t || t->type!=12 || t->inputs!=Width || t->outputs!=5120 || t->bytes/t->outputs!=Blocks*144)return false;
 const unsigned tensor=unsigned(t-model.Tensors().data());
 job.inputs=Width;job.bank=0;job.offset=0;
 for(unsigned row=0;row<16;row++)for(unsigned b=0;b<Blocks;b++){
  uint8_t block[144];
  if(!model.ReadPacked(tensor,(row*Blocks+b)*sizeof(block),sizeof(block),block,sizeof(block)))return false;
  for(unsigned i=0;i<sizeof(block);i++)data[b*L::TileBytes+i*16+row]=block[i];
 }
 for(unsigned token=0;token<2;token++){
  float reference[16]={};
  for(unsigned b=0;b<Blocks;b++){
   const float scale=token? (b%2?0x1p-10f:0.017578125f) : (b%3?-0.03125f:0.012345f);
   job.scale[token][b]=Bits(scale);
   for(unsigned p=0;p<16;p++){
    PioneerQ38Panel panel;
    if(!PioneerQ38UnpackPanel(12,data.data()+b*L::TileBytes,L::TileBytes,p,&panel))return false;
    int inputs[16],sum=0;
    for(unsigned k=0;k<16;k++){
     const unsigned i=b*256+p*16+k;
     inputs[k]=int((i*(token?71:29)+(token?163:11))%251)-125;sum+=inputs[k];
     job.activation[token][i/2]|=uint32_t(PioneerInt8Operand(inputs[k]))<<((i&1)*16);
    }
    for(unsigned col=0;col<16;col++){
     int dot=0;
     for(unsigned k=0;k<16;k++)dot+=Value(uint16_t(panel.operand[k*8+col/2]>>((col&1)*16)))*inputs[k];
     const float a=(Float(panel.coefficient[col])*scale)*float(dot);
     const float m=(Float(panel.minimum[col])*scale)*float(sum);
     reference[col]+=a+m;
    }
   }
  }
  for(unsigned i=0;i<16;i++)expected[token*16+i]=Bits(reference[i]);
 }
 unsigned different=0;
 for(unsigned i=0;i<16;i++)different+=expected[i]!=expected[i+16];
 return different && model.Stable();
}
int main(int argc,char** argv)
{
 if(argc!=4 || strcmp(argv[1],"--exercise"))return 2;
 char* end=nullptr;errno=0;
 const uint64_t generation=strtoull(argv[2],&end,10);
 if(errno || !generation || !end || *end || strspn(argv[2],"0123456789")!=strlen(argv[2]))return 2;
 auto request=std::make_unique<N::Request>();
 request->version=N::Version;request->tag=N::Tag;request->generation=generation;
 std::vector<uint8_t> data(Pages*4096);
 int input=open(argv[3],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
 if(input<0){fprintf(stderr,"Q8T4_FIXTURE_OPEN_FAILED errno%d;NO_CARD_OPEN\n",errno);return 1;}
 struct stat identity={};
 if(fstat(input,&identity) || !S_ISREG(identity.st_mode) || identity.st_size!=INT64_C(18973870528)){
  fprintf(stderr,"Q8T4_FIXTURE_IDENTITY_FAILED;NO_CARD_OPEN\n");close(input);return 1;
 }
 FILE* file=fdopen(input,"rb");
 if(!file){fprintf(stderr,"Q8T4_FIXTURE_STREAM_FAILED;NO_CARD_OPEN\n");close(input);return 1;}
 uint32_t expected[32]={};
 const bool fixture=Fixture(file,request->job,data,expected);
 if(fclose(file) || !fixture){
  fprintf(stderr,"Q8T4_FIXTURE_VALIDATION_FAILED;NO_CARD_OPEN\n");return 1;
 } // Entire CPU oracle and immutable fixture before card open.
 int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
 if(fd<0){fprintf(stderr,"Q8T4_CARD_OPEN_FAILED errno%d\n",errno);return 1;}
 tt_pioneer_driver_reload status={};status.version=1;status.expected_tag=N::Tag;status.expected_generation=generation;
 tt_q38_bank_support support={};tt_q38_bank_session session={};session.version=1;
 if(!Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&status,sizeof(status)) || status.guard_version!=1
  || status.generation!=generation || status.build_tag!=N::Tag || status.quiesced || status.cold
  || status.active_owner || status.open_count!=1
  || !Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support))
  || support.driver!=45 || support.version!=1 || support.banks!=7 || support.page_bytes!=4096
  || !support.storage_only || support.bytes_per_bank!=PioneerResidentBanks::Capacity
  || !Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) || session.result.version!=1
  || session.result.state!=TT_RESIDENT_SESSION_READY || !session.result.active || !session.result.host_ready
  || session.result.cold || session.result.capabilities || session.result.bytes!=PioneerResidentBanks::Total){
  fprintf(stderr,"Q8T4_BEGIN_FAILED tag%016llx generation%llu metadata_cold%u owner%u bank_abi%u state%u active%u host%u cold%u;NO_RETRY\n",
   (unsigned long long)status.build_tag,(unsigned long long)status.generation,status.cold,status.active_owner,
   support.version,session.result.state,session.result.active,session.result.host_ready,session.result.cold);
  close(fd);return 1;
 }
 auto page=std::make_unique<tt_q38_bank_page>();
 for(unsigned p=0;p<Pages;p++){
  *page={};page->version=1;page->offset=p*4096;
  memcpy(page->data,data.data()+page->offset,4096);
  if(!Call(fd,TT_PCI_PROBE_Q38_BANK_UPLOAD,page.get(),sizeof(*page)) || page->result.state!=TT_STORE_PASSED
   || !page->result.wrote_device || page->result.requests!=3 || page->result.completed!=3
   || page->result.mismatches || page->result.recovery_required){
   const auto& r=page->result;
   fprintf(stderr,"Q8T4_UPLOAD_FAILED page%u state%u wrote%u requests%u completed%u mismatches%u recovery%u;NO_RETRY\n",
    p,r.state,r.wrote_device,r.requests,r.completed,r.mismatches,r.recovery_required);
   close(fd);return 1; // no implicit END after uncertain upload
  }
 }
 const bool compared=Call(fd,N::CompareOperation,request.get(),sizeof(*request));
 if(close(fd) || !compared || request->cold || !N::Closed(request->cleanup)
  || request->compared!=1 || request->matched!=1 || request->generation!=generation || request->tag!=N::Tag){
  if(compared)ReportCompareFailure(*request);
  return 1;
 }
 for(unsigned i=0;i<32;i++)if(request->job.output[i]!=expected[i]){
  fprintf(stderr,"Q8T4_MISMATCH output%u;DEVICE_SESSION_CLOSED_NO_RATE\n",i);return 1;
 }
 for(unsigned a=0;a<N::Arms;a++){
  const auto& r=request->arms[a];
  if(!request->micros[a] || !r.started || !r.completed || !r.stopped || r.recovery_required
   || r.marker!=L::VectorMagic || r.panels!=Width/16){ReportCompareFailure(*request);return 1;}
  for(unsigned i=0;i<8;i++)if(uint32_t(r.after[i]-r.before[i])!=L::Delta(i)*Blocks){
   fprintf(stderr,"Q8T4_COUNTER_FAILED arm%u counter%u;DEVICE_SESSION_CLOSED_NO_RATE\n",a,i);return 1;
  }
 }
 printf("Q8T4_CLOSED K%u output32 exact32 bank0 GDDRreads%u/%u/%u wall_us%llu/%llu/%llu;"
  "SINGLE_SHARD_NOT_TOKEN_RATE\n",Width,Blocks,Blocks,Blocks,(unsigned long long)request->micros[0],
  (unsigned long long)request->micros[1],(unsigned long long)request->micros[2]);
 return 0;
}
