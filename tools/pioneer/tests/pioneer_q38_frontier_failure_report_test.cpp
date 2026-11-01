/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT 1
#define PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING 1
#define PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT 1
#define PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT 1
#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT 1
#define B_DEVICE_OP_CODES_END 0x270f
#include "../PioneerQ38FrontierFailureReport.h"
#include "../PioneerQ38FrontierFailureReportCli.h"
#include <array>
#include <cassert>
#include <climits>
#include <string>
namespace F=PioneerQ38TokenFrontierControl;
static std::string Contents(FILE* file){
 assert(!std::fflush(file) && !std::fseek(file,0,SEEK_END));const auto bytes=std::ftell(file);
 assert(bytes>=0 && bytes<2048 && !std::fseek(file,0,SEEK_SET));
 std::string text(size_t(bytes),'\0');assert(std::fread(&text[0],1,text.size(),file)==text.size());return text;
}
static F::Packet Unsafe(unsigned cursor=0){
 F::Packet p;auto& r=p.reply;r.version=1;r.operation=1;r.status=int32_t(0x8000a00cU);r.started=1;r.unsafe=1;
 r.detail[0]=r.detail[1]=r.detail[5]=1;r.detail[2]=12;r.detail[4]=cursor;return p;
}
static unsigned logs=0,refusals=0;
static void Check(const void* input,bool expectedShape,int status=-1,int error=int32_t(0x8000a00cU)){
 std::array<uint8_t,128> before;std::memcpy(before.data(),input,before.size());
 FILE* file=std::tmpfile();assert(file);
 assert(PioneerQ38WriteFrontierFailedReply(file,TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE,input,128,status,error));
 const auto text=Contents(file);std::fclose(file);
 assert(!std::memcmp(before.data(),input,before.size()));
 assert(text.find(std::string("consistent_shape")+(expectedShape?"1":"0"))!=std::string::npos);
 assert(text.find("ALREADY_RETURNED_DATA_NOT_PROVENANCE_ROOT_CAUSE_OR_CLEAN_END")!=std::string::npos
  && text.find("NOT_COMPLETED_LAYERS_RETIRED_PC_FAULT_CODE_OR_RESTORATION_AUTHORITY")!=std::string::npos);
 std::array<uint32_t,32> words;std::memcpy(words.data(),input,128);
 for(unsigned row=0;row<4;row++){
  char start[64];std::snprintf(start,sizeof(start),"Q38_FRONTIER_FAILED_PACKET word%02u",row*8);std::string line=start;
  for(unsigned column=0;column<8;column++){char word[16];std::snprintf(word,sizeof(word)," %08x",unsigned(words[row*8+column]));line+=word;}
  assert(text.find(line+"\n")!=std::string::npos);
 }
 assert(text.find("raw_phase"+std::to_string(words[18]))!=std::string::npos
  && text.find("raw_guard_cursor"+std::to_string(words[20]))!=std::string::npos);logs++;
}
int main(){
 static_assert(TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE==0x8d17 && sizeof(F::Packet)==128,"unchanged ABI");
 for(unsigned cursor=0;cursor<64;cursor++){auto p=Unsafe(cursor);assert(F::ReplyShape(p));Check(&p,true);}
 F::Packet pristine;assert(F::RequestShape(pristine));Check(&pristine,false); // ioctl may not have copied ANY reply
 auto p=Unsafe(63);std::array<uint32_t,32> original;std::memcpy(original.data(),&p,sizeof(p));
 for(unsigned word=0;word<32;word++){
  auto damaged=original;damaged[word]^=0x80000000U;
  // Flipping ONLY reply.status keeps the pre-existing consistency shape;
  // consistency MUST NOT be described as authentication or matching errno.
  Check(damaged.data(),word==10);
 }
 for(int status:{INT_MIN,-1,1,INT_MAX})Check(&p,true,status,INT_MAX);
 for(unsigned offset:{1U,2U,3U}){
  std::array<uint8_t,136> bytes;bytes.fill(0xa5);std::memcpy(bytes.data()+offset,&p,sizeof(p));const auto before=bytes;
  Check(bytes.data()+offset,true);assert(bytes==before);
 }
 FILE* file=std::tmpfile();assert(file);
 auto deny=[&](FILE* sink,uint32_t op,const void* data,size_t bytes,int status){
  assert(!PioneerQ38WriteFrontierFailedReply(sink,op,data,bytes,status,0));refusals++;
 };
 for(size_t bytes=0;bytes<=256;bytes++)if(bytes!=128)
  deny(file,0x8d17,reinterpret_cast<const void*>(uintptr_t(1)),bytes,-1);
 deny(file,0x8d17,reinterpret_cast<const void*>(uintptr_t(1)),SIZE_MAX,-1);
 for(uint32_t op:{0U,0x8d16U,0x8d18U,UINT32_MAX})deny(file,op,reinterpret_cast<const void*>(uintptr_t(1)),128,-1);
 deny(file,0x8d17,reinterpret_cast<const void*>(uintptr_t(1)),128,0); // SUCCESS must not read/log
 deny(file,0x8d17,nullptr,128,-1);deny(nullptr,0x8d17,&p,128,-1);
 assert(Contents(file).empty());std::fclose(file);
 using M=PioneerQ38FrontierFailureReportMode;
 const char* correct[]={"diagnostic","--diagnose-frontier-34-positions"};
 const char* source[]={"diagnostic","--preflight-source-only"};
 assert(PioneerQ38FrontierFailureReportArguments(2,correct)==M::Positions34
  && PioneerQ38FrontierFailureReportArguments(2,source)==M::SourceOnly);
 for(const char* value:{"--work","--cpu","--benchmark-34-positions","--profile-frontends-34-positions",
                       "--generate-34-positions","--capture-first-layer","--diagnose-frontier-34-positions ",""}){
  const char* bad[]={"diagnostic",value};assert(PioneerQ38FrontierFailureReportArguments(2,bad)==M::Invalid);refusals++;
 }
 for(int argc:{-1,0,1,3,INT_MAX})assert(PioneerQ38FrontierFailureReportArguments(argc,correct)==M::Invalid);
 const char* null0[]={nullptr,correct[1]},*null1[]={correct[0],nullptr};
 assert(PioneerQ38FrontierFailureReportArguments(2,nullptr)==M::Invalid
  && PioneerQ38FrontierFailureReportArguments(2,null0)==M::Invalid
  && PioneerQ38FrontierFailureReportArguments(2,null1)==M::Invalid);
 std::printf("PASS failed-reply RAW formatter/%u complete128B packets/%u no-read no-log refusals/ALL32 mutated words/pristine early rejection/misaligned input/unchanged caller bytes/explicit fixed34 CLI;NO transport,native,hardware,cause,cleanup,Work or TPS proof\n",logs,refusals);
}
