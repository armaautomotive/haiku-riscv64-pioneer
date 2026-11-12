/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38UploadTiming.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <limits>

static uint64_t tick=100;
static uint64_t Clock(){return tick;}
struct Pages {
 int part=7;bool okay=true;
 bool AllVerified()const{return true;}
 const int* Part(unsigned)const{return &part;}
 bool OutputSafe(const void* p,size_t n)const{return p && n;}
 bool ReadPages(unsigned,unsigned,unsigned count,uint8_t* out,unsigned cap,std::string&){
  assert(out && cap>=count*4096);tick+=30;std::memset(out,0x5a,count*4096);return okay;
 }
};
int main(){
 PioneerQ38UploadTiming stats(Clock);Pages pages;PioneerQ38TimedPages<Pages> timed(pages,stats);
 uint8_t buf[8192]={};std::string error;
 assert(timed.AllVerified() && timed.Part(0)==&pages.part && timed.OutputSafe(buf,sizeof(buf)));
 assert(!timed.OutputSafe(&timed,sizeof(timed)) && !timed.OutputSafe(&stats,sizeof(stats)));
 assert(!timed.ReadPages(0,0,1,reinterpret_cast<uint8_t*>(&timed),4096,error) && stats.readCalls==0);
 assert(timed.ReadPages(0,0,2,buf,sizeof(buf),error) && stats.valid && stats.readNs==30 && stats.readCalls==1 && stats.readBytes==8192 && buf[0]==0x5a);
 pages.okay=false;assert(!timed.ReadPages(0,0,1,buf,sizeof(buf),error) && stats.readCalls==2 && stats.readBytes==12288);
 const auto start=stats.Begin();tick+=17;stats.End(start,stats.callNs,stats.callCalls,stats.callBytes,4096);
 assert(stats.valid && stats.callNs==17 && stats.callCalls==1 && stats.callBytes==4096);
 tick=3;stats.End(4,stats.callNs,stats.callCalls,stats.callBytes,1);assert(!stats.valid && stats.callCalls==1);
 PioneerQ38UploadTiming overflow(Clock);overflow.readBytes=std::numeric_limits<uint64_t>::max();
 overflow.End(3,overflow.readNs,overflow.readCalls,overflow.readBytes,1);assert(!overflow.valid);
 PioneerQ38UploadTiming missing(nullptr);assert(!missing.valid);
 puts("PASS diagnostic page-read/ioctl wall-time accounting and refusal cases; NO device, DMA or bandwidth proof");
}
