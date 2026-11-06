/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_PREPARED_BF16_Q4_SOURCE_H
#define PIONEER_NATIVE_PREPARED_BF16_Q4_SOURCE_H
#include "PioneerNativeCompactQ4StreamSource.h"
#include "PioneerPreparedBf16Q4OriginalRows.h"
#include "PioneerQ38PreparedBf16Q4BlockData.h"
#include "PioneerQ38PreparedBf16Q4Reference.h"
#include "PioneerNativePreparedBf16Q4Data.h" // private compiled115 operands
// CPU-reference DIAGNOSTIC ONLY,never Work/fallback. ALL source binding and
// BOTH fresh references complete before GPU Open. Retains the SAME original
// FD through END;there is NO second pathname/open or runtime cache repair.
class PioneerNativePreparedBf16Q4Source {
 using Old=PioneerNativeCompactQ4StreamData;using Data=PioneerNativePreparedBf16Q4Data;
 PioneerNativeCompactQ4StreamSource original;
 std::array<std::array<uint32_t,32>,9> golden{};
 bool attempted=false,prepared=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
public:
 bool Ready()const{return prepared && original.Ready();}
 const uint32_t* Golden(unsigned sample)const{return sample<9 && Ready()?golden[sample].data():nullptr;}
 bool Prepare(const char* path,std::string& error){
  static_assert(Data::Version==1 && Data::Role==115 && Data::PreparedBytes==5632 && Data::Samples==9 && Data::Rows==32,"exact115 prepared data source required");
  if(attempted)return Fail(error,"Prepared115 source is one-shot.");
  attempted=true;
  if(!original.Prepare(path,error) || !original.Ready())return false;
  for(unsigned s=0;s<9;s++){
   const unsigned k=Old::Width(s),blocks=k/256,raw=blocks*1152,extra=blocks*1408;
   if(Data::Width(s)!=k || Data::Words(s)!=blocks*2688)return Fail(error,"Prepared115 source geometry differs.");
   for(unsigned w=0;w<raw;w++)if(Data::Word(s,w)!=Old::Word(s,w))return Fail(error,"Prepared115 original source differs.");
   for(unsigned w=0;w<blocks*128;w++)if(Data::Word(s,raw+extra+w)!=Old::Word(s,raw+w))return Fail(error,"Prepared115 input source differs.");
   for(unsigned b=0;b<blocks;b++){
    std::array<uint8_t,4608> rows{};std::array<uint16_t,256> input{};std::array<uint8_t,5632> encoded{};
    if(!original.Ready() || !PioneerPreparedBf16Q4OriginalRows::Block(s,b,rows,input)
     || !PioneerQ38PreparedBf16Q4BlockData::Prepare(rows.data(),rows.size(),encoded.data(),encoded.size()))return Fail(error,"Prepared115 original permutation refused.");
    for(unsigned w=0;w<1408;w++){
     const auto* p=encoded.data()+w*4;const uint32_t v=uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;
     if(Data::Word(s,raw+b*1408+w)!=v)return Fail(error,"Prepared115 compiled primary/mask differs from original.");
    }
    for(unsigned group=0;group<8;group++)if(!PioneerQ38PreparedBf16Q4Reference::Group(rows.data(),input.data(),group,golden[s].data()))return Fail(error,"Prepared115 fresh reference refused.");
   }
  }
  if(!original.Ready())return Fail(error,"Prepared115 original changed before publication.");
  prepared=true;error.clear();return true;
 }
};
#endif
