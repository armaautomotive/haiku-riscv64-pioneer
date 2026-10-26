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
namespace FfnResident {
static void InputPacket(unsigned packet,const uint32_t* d)
{
 if(packet>=3 || !Sealed(d) || !Idle())block_fault_park();
 const bool write=packet!=0;
 const unsigned local[]={0x90000,0x90c00,0x90e00},sizes[]={1024,512,64},index[]={10,12,13};
 unsigned size=sizes[packet],here=local[packet],remote=d[4]+d[index[packet]];
 const unsigned delta[]={write?0U:1U,write?0U:size/64,1,write?0U:1U,write?1U:0U,write?size/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++)expected[i]=*Words(0xffb20000+Counter(i))+delta[i];
 const uint32_t command[]={write?here:remote,0,write?d[9]:d[8],write?remote:here,0,write?d[8]:d[9],0,write?0x212U:0,size,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Idle() || !Sealed(d))block_fault_park();return;}
 }
 block_fault_park(); // terminal UNKNOWN,no later packet or command mutation
}
static void InputOnce(const uint32_t* d)
{
 InputPacket(0,d);
 PioneerQ38ActivationK q;PioneerQ38FfnNativeInput native;
 if(!PioneerQ38QuantizeK(reinterpret_cast<const float*>(0x90000),256,&q) || !PioneerQ38FfnPackNative(&q,&native))block_fault_park();
 volatile uint32_t* out=Words(0x90c00);for(unsigned i=0;i<128;i++)out[i]=native.operand[i];
 for(unsigned i=0;i<16;i++)out[128+i]=native.scale[i];
 volatile uint32_t* line=Words(0x90e00);line[1]=TT_Q38_FFN_INPUT_MARKER;line[2]=d[6];line[3]=d[5];line[4]=d[42];
 line[5]=d[40];line[6]=d[41];for(unsigned i=0;i<8;i++)line[7+i]=d[32+i];line[15]=0;
 InputPacket(1,d);InputPacket(2,d);
}
}
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
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
extern "C" void q38_projection_persistent_input(const uint32_t*,const uint32_t*,unsigned,unsigned);
#endif
extern "C" void q38_ffn_persistent_activation(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self)
{
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
 if(job[13]==5){q38_projection_persistent_input(job,source,worker,self);return;}
#endif
 using namespace FfnResident;
 uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 const bool input=job[13]==TT_Q38_FFN_PERSISTENT_INPUT;
 if(!(input?PioneerQ38FfnPersistentInputShape(job,d,source,worker,self):PioneerQ38FfnPersistentActivationShape(job,d,source,worker,self)) || !Sealed(d) || !Idle()
  || *Words(0x74100)!=job[12])block_fault_park();
 uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=*Words(0xffb20000+Counter(i));
 Words(0x74140)[0]=job[12];Words(0x74140)[4]=job[13];Fence();
 if(input)InputOnce(d);else if(q38_ffn_activation_once()!=256)block_fault_park();
 if(!Sealed(d) || !Idle()
  || *Words(0x74100)!=job[12])block_fault_park();
 uint32_t after[8];const unsigned delta[]={input?1U:2U,input?16U:32U,input?3U:4U,input?1U:2U,2,9,input?3U:4U,2};
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
 proof[48]=job[13];proof[49]=d[6];proof[50]=d[5];proof[51]=d[42];
 proof[52]=d[40];proof[53]=d[41];for(unsigned i=0;i<8;i++)proof[54+i]=d[32+i];
 proof[62]=d[0];proof[63]=1;Fence();
 Words(0x74140)[4]=0;Words(0x74140)[1]=job[12];Fence();
}
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
#if PIONEER_Q38_PROJECTION_PERSISTENT != 1
#error Exact separate projection persistent image role required
#endif
#include "PioneerQ38ProjectionInput.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
namespace ProjectionProducer {
using namespace FfnResident;
static __attribute__((noinline,optimize("Os"))) bool Seal(const uint32_t* d){
 PIONEER_Q38_MIXED_SEAL();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]!=d[i])return false;
 return true;
}
static __attribute__((noinline,optimize("Os"))) bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static __attribute__((noinline)) void Packet(unsigned p,const uint32_t* d){
 if(p>=3 || !Seal(d) || !Quiet())block_fault_park();
 bool write=p!=0;
 static const unsigned local[] __attribute__((section(".q38_projection_data")))={0x90000,0x90c00,0x90e00};
 static const unsigned size[] __attribute__((section(".q38_projection_data")))={1024,512,128};
 static const unsigned field[] __attribute__((section(".q38_projection_data")))={10,12,13};
 unsigned bytes=size[p],here=local[p],remote=d[4]+d[field[p]];uint32_t expected[8];
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 for(unsigned i=0;i<8;i++){
  expected[i]=*Words(0xffb20000+Counter(i))+delta[i];
  if(i==6 && expected[6]!=expected[2])block_fault_park();
 }
 const uint32_t command[]={write?here:remote,0,write?d[9]:d[8],write?remote:here,0,write?d[8]:d[9],0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Seal(d) || !Quiet())block_fault_park();return;}
 }
 block_fault_park(); // UNKNOWN:ZERO next packet/reset/restore/retry
}
}
extern "C" __attribute__((noinline,optimize("Os"))) void q38_projection_persistent_input(const uint32_t* job,const uint32_t* source,unsigned worker,unsigned self){
 using namespace FfnResident;uint32_t d[64];
 for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38ProjectionInputJob(job,d,source,worker,self) || !ProjectionProducer::Seal(d) || !ProjectionProducer::Quiet() || *Words(0x74100)!=job[12])block_fault_park();
 uint32_t before[8];for(unsigned i=0;i<8;i++){before[i]=*Words(0xffb20000+Counter(i));if(i==6 && before[6]!=before[2])block_fault_park();}
 Words(0x74140)[0]=job[12];Words(0x74140)[4]=5;Fence();
 ProjectionProducer::Packet(0,d);
 if(!PioneerQ38ProjectionQuantize(d[14],reinterpret_cast<const float*>(0x90000),256,
  reinterpret_cast<PioneerQ38ProjectionInput*>(0x90c00)))block_fault_park();
 volatile uint32_t* line=Words(0x90e00);
 const uint32_t fields[]={TT_Q38_PROJECTION_INPUT_MARKER,d[6],d[5],d[14],d[11],d[42],d[43],d[40],d[41]};
 for(unsigned i=0;i<9;i++)line[8+i]=fields[i];
 for(unsigned i=0;i<8;i++)line[17+i]=d[32+i];
 line[25]=d[15];line[26]=worker;line[27]=self;line[28]=source[0];line[29]=line[30]=0;line[31]=1;
 ProjectionProducer::Packet(1,d);ProjectionProducer::Packet(2,d);
 if(!ProjectionProducer::Seal(d) || !ProjectionProducer::Quiet() || *Words(0x74100)!=job[12])block_fault_park();
 uint32_t after[8];static const unsigned delta[] __attribute__((section(".q38_projection_data")))={1,16,3,1,2,10,3,2};
 for(unsigned i=0;i<8;i++){
  after[i]=*Words(0xffb20000+Counter(i));if(uint32_t(after[i]-before[i])!=delta[i])block_fault_park();
  if(i==6 && after[6]!=after[2])block_fault_park();
 }
 for(unsigned i=0;i<16;i++)if(Words(0x74000)[i]!=job[i] || *Words(0xffb20000+Source(i))!=source[i])block_fault_park();
 for(unsigned i=0;i<8;i++)if(Words(0x74040)[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?worker:i==2?self:i==3?source[0]:0))block_fault_park();
 volatile uint32_t* proof=Words(0x83f00);
 for(unsigned i=0;i<8;i++){proof[i]=before[i];proof[8+i]=after[i];}
 proof[16]=256;proof[17]=job[12];proof[18]=worker;proof[19]=TT_Q38_FFN_PERSISTENT_MARKER;
 for(unsigned i=0;i<10;i++)proof[20+i]=Words(0xffb20000)[i];
 proof[30]=source[0];proof[31]=self;for(unsigned i=0;i<16;i++)proof[32+i]=source[i];
 proof[48]=5;proof[49]=d[6];proof[50]=d[5];proof[51]=d[42]|(d[43]<<8)|(d[14]<<16);proof[52]=d[40];proof[53]=d[41];
 for(unsigned i=0;i<8;i++)proof[54+i]=d[32+i];
 proof[62]=TT_Q38_PROJECTION_INPUT_MARKER;proof[63]=d[11];Fence();
 Words(0x74140)[4]=0;Words(0x74140)[1]=job[12];Fence();
}
#endif
