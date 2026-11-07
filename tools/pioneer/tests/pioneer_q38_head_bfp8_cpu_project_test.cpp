/* SPDX-License-Identifier: MIT */
#define PIONEER_HEAD_FIXTURE_ONLY
#include "pioneer_q38_head_block_float_test.cpp"
#include "pioneer_q38_head_bfp8_cpu_project.h"

int main(){
 HeadSource source,reference;std::vector<uint32_t> input(5120),actual;
 for(unsigned i=0;i<input.size();i++){float x=float(int(i%19)-9)/16;std::memcpy(&input[i],&x,4);}
 assert(Q38HeadBfp8CpuProject(source,0,input,0,2,actual) && actual.size()==64 && source.reads==64);
 for(unsigned band=0;band<2;band++){
  std::vector<uint8_t> packed;std::array<uint32_t,32> expected={};
  assert(PioneerQ38HeadBlockFloatDiagnostic::PackBand(reference,0,band*32,PioneerBlackholeBlockFloat::Format::Bfp8B,packed));
  assert(PioneerBfp4PromptQuality::PredictFormat(PioneerBlackholeBlockFloat::Format::Bfp8B,packed.data(),packed.size(),input.data(),5120,expected));
  assert(std::equal(expected.begin(),expected.end(),actual.begin()+band*32));
 }
 HeadSource broken;broken.fail=33;std::vector<uint32_t> untouched(3,0x12345678),before=untouched;
 assert(!Q38HeadBfp8CpuProject(broken,0,input,0,2,untouched) && untouched==before);
 HeadSource invalid;auto bad=input;bad[0]=0x7f800000;
 assert(!Q38HeadBfp8CpuProject(invalid,0,bad,0,1,untouched) && untouched==before && !invalid.reads);
 assert(!Q38HeadBfp8CpuProject(invalid,0,input,1,1,untouched) && untouched==before && !invalid.reads);
 HeadSource revoked;unsigned completed=0;
 assert(!Q38HeadBfp8CpuProject(revoked,0,input,0,2,untouched,[&](unsigned done,unsigned total){
  assert(total==2);completed=done;revoked.ready=false;
 }) && completed==1 && untouched==before && revoked.reads==32);
 puts("PASS head BFP8 CPU observation two synthetic bands, exact software arithmetic and atomic refusals; no device or TPS");
}
