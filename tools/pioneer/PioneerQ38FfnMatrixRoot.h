/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_MATRIX_ROOT_H
#define PIONEER_Q38_FFN_MATRIX_ROOT_H
#ifndef PIONEER_Q38_FFN_RESIDENT_MATRIX
#error Private offline FFN root only
#endif
// Included AFTER the existing dispatcher's fixed Words/Idle/source/counter
// functions. No new host address,PCI operation,peer code/reset or cache hint.
namespace FfnMatrix {
static __attribute__((noinline)) bool Sealed(const uint32_t* d,const uint32_t* first)
{
 bool same=true;for(unsigned i=0;i<16;i++)same &= *Words(0xffb20000+SourceOffset(i))==d[16+i];
 for(unsigned i=0;i<64;i++)same &= Words(TT_Q38_FFN_MATRIX_FRAME)[i]==d[i];
 for(unsigned i=0;i<119;i++)same &= Words(TT_Q38_FFN_MATRIX_ROWS)[i]==first[i];
 return same;
}
static __attribute__((noinline)) void Packet(bool write,unsigned local,unsigned remote,unsigned bytes,
 const uint32_t* d,const uint32_t* first)
{
 // These arguments come ONLY from the fixed geometry in Fetch/Scatter,
 // not user parameters. Shape/complete row cover was validated before IO.
 if(!bytes || bytes>16384 || (bytes&63) || !Sealed(d,first) || !Idle())block_fault_park();
 const uint32_t command[]={write?local:remote,0,write?129U:d[8],write?remote:local,0,write?d[8]:129U,0,write?0x212U:0,bytes,0};
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++)expected[i]=*Words(0xffb20000+Offset(i))+delta[i];
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Offset(i))==expected[i];
  if(complete){if(!Sealed(d,first) || !Idle())block_fault_park();
   Words(0x7f800)[write?7:6]++;Words(0x7f800)[write?9:8]+=bytes/64;
   Words(0x7f800)[write?52:50]++;Words(0x7f800)[write?53:51]+=bytes/64;Fence();return;}
 }
 block_fault_park(); // UNKNOWN:ZERO later IO/hold/restore/retry
}
static __attribute__((noinline)) void Fetch(const uint32_t* d,const uint32_t* first)
{
 unsigned bytes=d[14]*2,offset=d[5]==2?239744:20480;
 for(unsigned at=0;at<bytes;at+=16384)Packet(false,0xa0000+at,d[4]+offset+at,
  bytes-at>16384?16384:bytes-at,d,first);
 Packet(false,0xab000,d[4]+(d[5]==2?274560:30720),d[5]==2?320:128,d,first);
 // The caller MUST validate ALL fetched native operands/padded scales
 // before staging any peer descriptor/doorbell. This is NOT old reuse2.
}
static __attribute__((noinline)) void Scatter(const uint32_t* d,const uint32_t* first,const uint32_t parts[119][8],unsigned packed)
{
 if(packed!=d[11] || !Sealed(d,first) || !Idle())block_fault_park();
 if(PioneerQ38FfnMatrixOrdered(parts,first,d[7])) {
  // Canonical sorted row plans need only5/2 NoC writes,not119. No CPU
  // reorder/copy/roundtrip;all bytes already compact in global row order.
  for(unsigned at=0;at<d[12];at+=16384)Packet(true,0x80000+at,d[4]+d[10]+at,
   d[12]-at>16384?16384:d[12]-at,d,first);
 } else {
  unsigned at=0;for(unsigned w=0;w<d[7];w++) {
   unsigned bytes=parts[w][2]*64;Packet(true,0x80000+at,d[4]+d[10]+first[w]*4,bytes,d,first);at+=bytes;
  }
 }
 if(!Sealed(d,first) || !Idle())block_fault_park();
 Words(0x7f800)[49]=TT_Q38_FFN_MATRIX_MARKER;Words(0x7f800)[54]=d[5];Words(0x7f800)[55]=d[11];Words(0x7f800)[56]=1;Fence();
}
}
#endif
