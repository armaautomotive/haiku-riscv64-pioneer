/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_Q4_LAYOUT_H
#define PIONEER_Q38_PREPARED_Q4_LAYOUT_H
// Pure DATA/geometry for a DISTINCT prepared-source worker. These addresses
// grant NO native mapping,NoC,root-table lifetime or source ownership.
struct PioneerQ38PreparedQ4Layout {
 using Word=__UINT32_TYPE__;
 static constexpr unsigned Magic=0x31513450,SummaryMagic=0x31533450;
 static constexpr unsigned RootTable=0xab400,Descriptor=0x93600,Summary=0x93700;
 static constexpr unsigned Index=0x93000,IndexCapacity=512,Payload=0x8a000,PayloadCapacity=0x8900;
 static constexpr unsigned DenseMask=0x9c400,InputFirst=0x96200,InputFirstBytes=4608,InputSecond=0xa8900;
 static constexpr unsigned OffsetMask=0x3fffffff,SourceBegin=0x04000000,SourceEnd=0xc4000000;
 struct Window{unsigned start,bytes;Window(unsigned first=0,unsigned count=0):start(first),bytes(count){}};
 static constexpr unsigned Align64(unsigned n){return (n+63)&~63U;}
 static constexpr unsigned Allocated(unsigned n){return (n+4095)&~4095U;}
 static constexpr unsigned RootSlot(unsigned worker){return worker && worker<120?RootTable+(worker-1)*64:0;}
 static constexpr unsigned Input(unsigned word){return word<InputFirstBytes/4?InputFirst+word*4:InputSecond+(word-InputFirstBytes/4)*4;}
 static bool Shape(const volatile Word* d,const volatile Word* j){
  if(!d || !j || j[1]!=12 || j[2]!=2304 || j[14]!=1 || !RootSlot(j[11]) || !j[12] || j[12]>=0x7fffffff
   || d[0]!=Magic || d[1]!=j[11] || d[2]!=j[12] || d[3]!=12 || d[4]!=j[5] || d[5]!=j[7]
   || d[6]!=j[6] || d[7]!=j[8] || d[14]!=1 || d[15]
   || (j[5]!=5120 && j[5]!=6144 && j[5]!=17408) || !j[7] || j[7]>16)return false;
  const unsigned tiles=d[11],blocks=j[5]>>8;
  if(!tiles || tiles>(SourceEnd-SourceBegin)/2304 || d[10]>=tiles || j[7]*blocks>tiles-d[10]
   || d[12]<SourceBegin || d[12]>=SourceEnd || (d[12]&4095) || d[13]!=tiles*2304 || d[13]>SourceEnd-d[12]
   || d[6]!=d[12]+d[10]*2304 || d[8]<SourceBegin || d[8]>=SourceEnd || (d[8]&4095)
   || d[8]<d[12]+Allocated(d[13]) || !d[9] || (d[9]&15) || d[9]>OffsetMask
   || d[9]<((tiles*4+15)&~15U) || Allocated(d[9])>SourceEnd-d[8])return false;
  return true;
 }
 static unsigned IndexSource(const volatile Word* d,unsigned group){return d[8]+((d[10]+group*(d[4]>>8))*4&~63U);}
 static unsigned IndexWordsBefore(const volatile Word* d,unsigned group){return (d[10]+group*(d[4]>>8))&15;}
 static unsigned IndexReadBytes(const volatile Word* d,unsigned group){return Align64((IndexWordsBefore(d,group)+(d[4]>>8))*4);}
 static bool Span(unsigned kind,unsigned bytes){return kind==0?bytes==512:kind==1 && bytes>=48 && bytes<=512 && !(bytes&15);}
 static bool GroupWindow(const volatile Word* d,unsigned group,const volatile Word* index,Window& output){
  if(group>=d[5] || IndexReadBytes(d,group)>IndexCapacity)return false;
  const unsigned first=IndexWordsBefore(d,group),blocks=d[4]>>8,minimum=(d[11]*4+15)&~15U;
  unsigned start=0,last=0,previousKind=3;
  for(unsigned b=0;b<blocks;b++){
   const unsigned value=index[first+b],kind=value>>30,at=value&OffsetMask;
   if(kind==2){if(at)return false;continue;}
   if(kind>1 || at<minimum || (at&15) || at>d[9] || (kind==0?512U:34U)>d[9]-at)return false;
   if(start && (at<=last || !Span(previousKind,at-last)))return false;
   if(!start)start=at;
   last=at;previousKind=kind;
  }
  if(!start){output={0,0};return true;}
  const unsigned begin=start&~63U,end=Align64(last+512>d[9]?d[9]:last+512);
  if(end<begin || end>Allocated(d[9]) || end-begin>PayloadCapacity)return false;
  output={begin,end-begin};return true;
 }
 static bool SummaryShape(const volatile Word* s,const Word* j,unsigned& reads,unsigned& flits){
  if(s[0]!=SummaryMagic || s[1]!=j[11] || s[2]!=j[12] || s[3]!=j[5] || s[4]!=j[7]
   || s[5]<1+j[7] || s[5]>1+j[7]*4 || s[6]<1+j[7] || s[6]>1+j[7]*554)return false;
  for(unsigned i=7;i<16;i++)if(s[i])return false;
  reads=s[5];flits=s[6];return true;
 }
};
static_assert(PioneerQ38PreparedQ4Layout::RootTable+119*64<0xb0000
 && PioneerQ38PreparedQ4Layout::Payload+PioneerQ38PreparedQ4Layout::PayloadCapacity<0x93000
 && PioneerQ38PreparedQ4Layout::InputFirst+4608+224<0x97ffc
 && PioneerQ38PreparedQ4Layout::InputSecond+30208+224<=0xb0000,"bounded distinct prepared-source DATA extents");
#endif
