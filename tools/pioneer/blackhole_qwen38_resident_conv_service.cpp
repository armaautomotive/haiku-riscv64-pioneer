/* SPDX-License-Identifier: MIT */
// Separate OFFLINE returning Conv service. No native registry selects it.
#include "PioneerQ38ResidentOperatorImageRole.h"
#define scalar_main q38_resident_conv_once
#define scalar_fault_park block_fault_park
#include "blackhole_qwen38_conv_batch_silu_precision.cpp"
#undef scalar_main
#undef scalar_fault_park
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentConvServiceShape.h"
extern "C" void q38_resident_persistent_conv(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self){
 using namespace Conv;
 uint32_t table[384],before[8],after[8],summary[32];
 for(unsigned i=0;i<384;i++)table[i]=Words(TT_Q38_CONV_BATCH_TABLE)[i];
 if(!PioneerQ38ResidentConvServiceJob(job,table,source,worker,self) || !Seal(table,table) || !Quiet()
  || *Words(0x74100)!=job[12])block_fault_park();
 for(unsigned i=0;i<8;i++){before[i]=*Words(0xffb20000+Counter(i));if(i==6 && before[i]!=before[2])block_fault_park();}
 Words(0x74140)[0]=job[12];Words(0x74140)[4]=TT_Q38_RESIDENT_CONV_SERVICE_OPERATION;Fence();
 unsigned n=PioneerQ38ConvBatchCount(worker);
 if(q38_resident_conv_once()!=16*n || !Seal(table,table) || !Quiet() || *Words(0x74100)!=job[12])block_fault_park();
 for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<32;i++)summary[i]=Words(TT_Q38_FFN_RESIDENT_SUMMARY)[i];
 if(!PioneerQ38ConvBatchSummary(summary,table,before,after))block_fault_park();
 for(unsigned i=0;i<16;i++)if(Words(0x74000)[i]!=job[i] || table[16+i]!=source[i])block_fault_park();
 for(unsigned i=0;i<8;i++)if(Words(0x74040)[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?worker:i==2?self:i==3?source[0]:0))block_fault_park();
 const auto* d=table+(n-1)*64;
 const uint32_t command[]={0x81000,0,self,d[4]+d[43],0,d[8],0,0x212,64,0};
 for(unsigned i=0;i<10;i++)if(*Words(0xffb20000+i*4)!=command[i])block_fault_park();
 volatile uint32_t* proof=Words(0x83f00);
 for(unsigned i=0;i<8;i++){proof[i]=before[i];proof[8+i]=after[i];}
 proof[16]=16*n;proof[17]=job[12];proof[18]=worker;proof[19]=TT_Q38_FFN_PERSISTENT_MARKER;
 for(unsigned i=0;i<10;i++)proof[20+i]=command[i];
 proof[30]=source[0];proof[31]=self;for(unsigned i=0;i<16;i++)proof[32+i]=source[i];
 proof[48]=TT_Q38_RESIDENT_CONV_SERVICE_OPERATION;proof[49]=table[6];proof[50]=table[5];proof[51]=table[7];
 proof[52]=table[40];proof[53]=0;for(unsigned i=0;i<8;i++)proof[54+i]=table[32+i];
 proof[62]=TT_Q38_CONV_BATCH_MARKER;proof[63]=n;Fence();
 Words(0x74140)[4]=0;Words(0x74140)[1]=job[12];Fence();
 // Return into the SAME persistent sequence loop. No reset/host callback.
}
