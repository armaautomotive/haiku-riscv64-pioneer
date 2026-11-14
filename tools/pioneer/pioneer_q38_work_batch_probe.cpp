/* SPDX-License-Identifier: MIT */
// Fixed synthetic DEBUG qualification. CPU reference is present only here,
// never in the no-reference Work client. No model or speed claim.
#define PIONEER_Q38_WORK_PROBE_MAIN BatchUnusedWorkMain
#include "pioneer_q38_work_probe.cpp"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkBatchProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"

#ifndef PIONEER_Q38_BATCH_PROBE_MAIN
#define PIONEER_Q38_BATCH_PROBE_MAIN main
#endif
int PIONEER_Q38_BATCH_PROBE_MAIN(int argc,char** argv)
{
 if(argc!=2 || strcmp(argv[1],"--fixed-synthetic"))return 2;
 (void)&BatchUnusedWorkMain;
 constexpr unsigned workers=2,inputs=5120,type=8;
 auto packet=std::make_unique<PioneerQ38WorkBatchPacket>();
 packet->version=PIONEER_Q38_WORK_BATCH_VERSION;packet->count=PIONEER_Q38_WORK_BATCH_COUNT;
 auto& job=packet->jobs[0];job.version=1;job.type=type;job.inputs=inputs;job.peers=workers;
 const unsigned blocks=inputs/PioneerQ38CohortBlockInputs(type),tileBytes=PioneerQ38PanelTileBytes(type);
 const unsigned groupBytes=blocks*tileBytes,head=64;
 std::vector<float> expected(workers*256,0);
 std::vector<std::vector<uint8_t>> upload(workers);
 for(unsigned k=0;k<inputs;k++)job.activation[k/2]|=uint32_t(PioneerInt8Operand(int((k*29+17)%256)-128))<<((k%2)*16);
 for(unsigned b=0;b<blocks;b++)job.scales[b]=PioneerAttentionBits(b%3?-.03125f:.012345f);
 for(unsigned w=0;w<workers;w++){
  job.part[w]={w,1,head};std::vector<uint8_t> packed(groupBytes,0);
  for(unsigned b=0;b<blocks;b++)for(unsigned row=0;row<16;row++){
   uint8_t* tile=packed.data()+b*tileBytes;
   for(unsigned byte=0;byte<34;byte++)tile[byte*16+row]=uint8_t(byte*37+row*59+b*13+w*23);
   const uint16_t half=row%3?uint16_t(0x3c00+row*13):uint16_t(1+row);
   tile[row]=uint8_t(half);tile[16+row]=uint8_t(half>>8);tile[32+row]=128;
  }
  if(!Reference(job,packed,w,expected.data()+w*256))return 1;
  upload[w].resize(head+groupBytes);memcpy(upload[w].data()+head,packed.data(),packed.size());
 }
 packet->jobs[1]=job;
 auto inputTemplate=std::make_unique<tt_q38_dispatch_probe>(job);
 int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);if(fd<0)return 1;
 tt_pioneer_driver_reload identity={};identity.version=1;PioneerQ38WorkBatchSupport batch={};
 tt_q38_dispatch_support work={};tt_q38_bank_support storage={};
 bool okay=Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&identity,sizeof(identity)) && identity.version==1
  && identity.driver==55 && identity.build_tag==0x2026111300010152ULL && identity.generation
  && !identity.quiesced && !identity.cold && !identity.active_owner && identity.guard_version==1
  && Call(fd,TT_PCI_PROBE_Q38_WORK_BATCH_SUPPORT,&batch,sizeof(batch))
  && batch.version==1 && batch.count==2 && batch.driver==55 && !batch.reserved && batch.tag==identity.build_tag
  && Call(fd,TT_PCI_PROBE_Q38_WORK_SUPPORT,&work,sizeof(work)) && work.driver==55 && work.version==TT_Q38_WORK_SUPPORT_VERSION
  && work.peers==119 && work.rows==256 && work.max_inputs==17408 && work.banks==7 && work.offset_alignment==64
  && work.persistent==1 && !work.model_inference
  && Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&storage,sizeof(storage)) && storage.driver==45 && storage.version==1
  && storage.banks==7 && storage.page_bytes==4096 && storage.max_pages==4 && storage.storage_only==1
  && storage.bytes_per_bank==PioneerResidentBanks::Capacity;
 if(!okay){close(fd);return 1;} // metadata only, NO BEGIN
 tt_q38_bank_session session={};session.version=1;
 okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) && session.result.version==1
  && session.result.state==TT_RESIDENT_SESSION_READY && session.result.active==1 && session.result.host_ready==1
  && !session.result.cold && !session.result.capabilities && session.result.bytes==PioneerResidentBanks::Total;
 for(unsigned w=0;w<workers && okay;w++)okay=Upload(fd,w,0,upload[w]);
 if(okay)okay=Call(fd,TT_PCI_PROBE_Q38_WORK_BATCH_RUN,packet.get(),sizeof(*packet));
 PioneerQ38WorkHistory history;
 if(okay)okay=packet->version==1 && packet->count==2 && !packet->reserved[0] && !packet->reserved[1];
 for(unsigned i=0;i<2 && okay;i++)okay=!memcmp(&packet->jobs[i],inputTemplate.get(),offsetof(tt_q38_dispatch_probe,result))
  && history.Accept(packet->jobs[i]) && !ReturnedNumericErrors(packet->jobs[i],expected,stderr);
 if(!okay){fprintf(stderr,"STOP_BATCH_DEBUG_UNCERTAIN;CLOSE_ONLY_NO_END_RETRY_RESET\n");close(fd);return 1;}
 session={};session.version=1;
 okay=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) && session.result.version==1
  && session.result.state==TT_RESIDENT_SESSION_CLOSED && !session.result.active
  && session.result.pci_restored==1 && session.result.host_released==1 && !session.result.cold;
 if(close(fd))okay=false;
 if(!okay)return 1;
 puts("PASS_TWO_WAVE_BATCH_SYNTHETIC_REFERENCE_EXACT_KNOWN_CLEAN;NO_MODEL_OR_SPEED_CLAIM");
 return 0;
}
