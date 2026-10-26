/* SPDX-License-Identifier: MIT */
// Actual returning arithmetic component in a NEW persistent ROOT shell.
// This shell supplies no matrix/GDN/full-model/native authority by itself.
#define scalar_main q38_mixed_dense_frame
#include "blackhole_qwen38_dense_partition_root.cpp"
#undef scalar_main
extern "C" unsigned mixed_root_wait(unsigned);
extern "C" void q38_mixed_root_seal(){
 using namespace DensePartitionRoot;const L::PrivateSeal* p;
 asm volatile("mv %0,s11":"=r"(p));
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x6fe00)scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(L::Boot)[i]!=p->boot[i] || Words(L::Header)[i]!=p->frame[i])scalar_fault_park();
 if(*Words(L::Doorbell)!=p->frame[48])scalar_fault_park();
}
extern "C" void mixed_root_main(){
 using namespace DensePartitionRoot;L::PrivateSeal seal;uint32_t counters[8];
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(L::Boot)[i];
 if(!L::BootShape(seal.boot) || *Words(L::Doorbell) || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=seal.boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
 if(counters[2]!=counters[6])scalar_fault_park();
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 for(unsigned i=0;i<16;i++)Words(L::Status)[i]=0;
 Words(L::Status)[3]=L::Magic;Fence();unsigned previous=0;
 for(;;){
  unsigned sequence=mixed_root_wait(previous);
  for(unsigned i=0;i<64;i++)seal.frame[i]=Words(L::Header)[i];
  if(!sequence || sequence>=0x7fffffff || sequence!=previous+1 || seal.frame[48]!=sequence || !L::Shape(seal.frame)
   || seal.frame[5]!=seal.boot[3])scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(seal.frame[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(seal.frame[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  q38_mixed_root_seal();Words(L::Status)[0]=sequence;Words(L::Status)[4]=1;Fence();
  if(q38_mixed_dense_frame()!=6144)scalar_fault_park();
  q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
  for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
  if(counters[2]!=counters[6])scalar_fault_park();
  previous=sequence;Words(L::Status)[5]=seal.frame[3];Words(L::Status)[6]=seal.frame[4];Words(L::Status)[7]=6144;
  Words(L::Status)[4]=0;Fence();Words(L::Status)[1]=previous;Fence();
 }
}
