/* SPDX-License-Identifier: MIT */
// Offline/CPU-only candidate measurement. Never changes an upload or driver.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerResidentStore.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>

static uint64_t Now(){timespec t{};assert(!clock_gettime(CLOCK_MONOTONIC,&t));return uint64_t(t.tv_sec)*1000000000+t.tv_nsec;}
int main(int argc,char** argv)
{
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--bench")));
 uint32_t data[4096],expected[4],actual[4],state=0x12345678;
 for(unsigned trial=0;trial<128;trial++){
  for(auto& value:data){state^=state<<13;state^=state>>17;state^=state<<5;value=state;}
  for(unsigned page=0;page<4;page++)expected[page]=PioneerResidentPageHash(data+page*1024);
  PioneerResidentPageHashFour(data,actual);assert(!memcmp(expected,actual,sizeof(actual)));
 }
 puts("PASS batch candidate matches existing FNV bookkeeping for 512 distinct synthetic pages; NO driver or device");
 if(argc==2){
  uint32_t original[4096];memcpy(original,data,sizeof(data));
  volatile uint32_t serial=0,batched=0;const unsigned repeats=4096;uint64_t begin=Now();
  for(unsigned n=0;n<repeats;n++){
   data[n&4095]^=n;for(unsigned p=0;p<4;p++)serial+=PioneerResidentPageHash(data+p*1024);
  }
  const uint64_t middle=Now();
  memcpy(data,original,sizeof(data));
  for(unsigned n=0;n<repeats;n++){
   data[n&4095]^=n;PioneerResidentPageHashFour(data,actual);for(auto value:actual)batched+=value;
  }
  const uint64_t end=Now();
  assert(middle>=begin && end>=middle && serial==batched);
  printf("CPU_ONLY_64MIB_SERIAL_NS %llu BATCH4_NS %llu CHECKSUM %u; NO DEVICE OR THROUGHPUT CLAIM\n",
   (unsigned long long)(middle-begin),(unsigned long long)(end-middle),unsigned(serial));
 }
}
