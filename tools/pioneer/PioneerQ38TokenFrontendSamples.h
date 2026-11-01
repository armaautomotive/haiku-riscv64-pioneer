/* SPDX-License-Identifier: MIT */
#ifdef _KERNEL_MODE
#error USER snapshot DATA is not native/kernel authority
#endif
#ifndef PIONEER_Q38_TOKEN_FRONTEND_SAMPLES_H
#define PIONEER_Q38_TOKEN_FRONTEND_SAMPLES_H
#include <array>
#include <cstdint>
// Raw addresses already returned by an existing successful Observe call.
// They are NOT retired PCs,layer numbers,cycles or completion authority.
struct PioneerQ38TokenFrontendSnapshot {
 uint64_t generation=0;
 uint32_t ready=0,stage=0,checkedWord=0,firstPC=0,secondPC=0,reserved=0;
};
static_assert(sizeof(PioneerQ38TokenFrontendSnapshot)==32,"fixed USER observation DATA");
class PioneerQ38TokenFrontendSamples {
public:
 enum {Capacity=18000};
 struct Row {int64_t begin=0,end=0;uint32_t precedingCount=0;PioneerQ38TokenFrontendSnapshot observed;};
private:
 std::array<Row,Capacity> rows={};unsigned used=0;bool valid=true,terminal=false;
 bool Fail(){valid=false;return false;}
public:
 bool Observe(int64_t begin,int64_t end,unsigned precedingCount,const PioneerQ38TokenFrontendSnapshot& observed){
  if(!valid)return false;
  if(terminal || used==Capacity || begin<0 || end<begin || precedingCount>34
   || !observed.generation || observed.reserved || observed.ready>1
   || observed.stage!=(observed.ready?11U:9U) || (!observed.ready && observed.checkedWord>63)
   || (used && (begin<rows[used-1].end || precedingCount<rows[used-1].precedingCount
    || observed.generation!=rows[0].observed.generation)))return Fail();
  rows[used++]={begin,end,precedingCount,observed};terminal=observed.ready!=0;return true;
 }
 bool Complete()const{return valid && terminal && used;}
 unsigned Size()const{return used;}
 uint64_t Generation()const{return used?rows[0].observed.generation:0;}
 const Row* Data()const{return rows.data();}
};
static_assert(sizeof(PioneerQ38TokenFrontendSamples)<2*1024*1024,"USER heap only,not a kernel stack buffer");
#endif
