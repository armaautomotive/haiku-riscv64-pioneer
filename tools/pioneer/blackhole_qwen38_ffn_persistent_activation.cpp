/* SPDX-License-Identifier: MIT */
// OFFLINE persistent peer operation ONLY. No native registry selects it.
// Reuse the independently tested local arithmetic/packet implementation,
// but RETURN to the same persistent loop,never reset its sequence history.
#define scalar_main q38_ffn_activation_once
#define scalar_fault_park block_fault_park
#include "blackhole_qwen38_ffn_resident_activation.cpp"
#undef scalar_main
#undef scalar_fault_park
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentShape.h"
extern "C" void q38_ffn_persistent_matrix_begin(const uint32_t* source,unsigned self)
{
 using namespace FfnResident;
 if(!Idle())block_fault_park();
 uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=source[i])block_fault_park();
 // A completed activation ends with a GDDR WRITE command. Restore the
 // bounded matrix READ header only at the NEXT operation,so the root can
 // first audit the actual completed sink command. No submit/NoC packet.
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=i==5?self:0;
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=(i==5?self:0))block_fault_park();
 Fence();
 if(!Idle() || *Words(0xffb20014)!=self)block_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=source[i])block_fault_park();
 for(unsigned i=0;i<8;i++)if(*Words(0xffb20000+Counter(i))!=before[i])block_fault_park();
}
extern "C" void q38_ffn_persistent_activation(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self)
{
 using namespace FfnResident;
 uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38FfnPersistentActivationShape(job,d,source,worker,self) || !Sealed(d) || !Idle()
  || *Words(0x74100)!=job[12])block_fault_park();
 uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=*Words(0xffb20000+Counter(i));
 Words(0x74140)[0]=job[12];Words(0x74140)[4]=TT_Q38_FFN_PERSISTENT_ACTIVATION;Fence();
 if(q38_ffn_activation_once()!=256 || !Sealed(d) || !Idle()
  || *Words(0x74100)!=job[12])block_fault_park();
 uint32_t after[8];const unsigned delta[]={2,32,4,2,2,9,4,2};
 for(unsigned i=0;i<8;i++) {
  after[i]=*Words(0xffb20000+Counter(i));
  if(uint32_t(after[i]-before[i])!=delta[i] || d[16+i]!=source[i] || d[24+i]!=source[8+i])block_fault_park();
 }
 for(unsigned i=0;i<16;i++)if(Words(0x74000)[i]!=job[i])block_fault_park();
 for(unsigned i=0;i<8;i++)if(Words(0x74040)[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?worker:i==2?self:i==3?source[0]:0))block_fault_park();
 // Command word5 is a mutable destination,NOT independent self identity.
 // New operation trailer is DATA;root still owes every physical/current
 // participant/owner proof before it may consume this GDDR producer output.
 volatile uint32_t* proof=Words(0x83f00);
 for(unsigned i=0;i<8;i++){proof[i]=before[i];proof[8+i]=after[i];}
 proof[16]=256;proof[17]=job[12];proof[18]=worker;proof[19]=TT_Q38_FFN_PERSISTENT_MARKER;
 for(unsigned i=0;i<10;i++)proof[20+i]=Words(0xffb20000)[i];
 proof[30]=source[0];proof[31]=self;for(unsigned i=0;i<16;i++)proof[32+i]=source[i];
 proof[48]=TT_Q38_FFN_PERSISTENT_ACTIVATION;proof[49]=d[6];proof[50]=d[5];proof[51]=d[42];
 proof[52]=d[40];proof[53]=d[41];for(unsigned i=0;i<8;i++)proof[54+i]=d[32+i];
 proof[62]=TT_Q38_FFN_RESIDENT_MARKER;proof[63]=1;Fence();
 Words(0x74140)[4]=0;Words(0x74140)[1]=job[12];Fence();
}
