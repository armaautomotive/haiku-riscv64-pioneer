/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#include "pioneer_q38_native_peer_test_bridge.h"
#include <cassert>
#include <cstdio>

struct Rom {unsigned calls=0;};
static bool Program(void* context,unsigned worker,uint32_t address,uint32_t& output){
 auto& rom=*static_cast<Rom*>(context);rom.calls++;
 if(worker!=1 || address!=0x20000)return false;output=0x12345678;return true;
}
int main(){
 Rom rom;Q38NativePeerTestBytes bytes;bytes.context=&rom;bytes.programWord=Program;uint32_t word=0xa5a5a5a5;
 assert(Q38NativePeerTestProgramWord(bytes,1,0x20000,word) && word==0x12345678 && rom.calls==1);
 word=0xa5a5a5a5;assert(!Q38NativePeerTestProgramWord(bytes,2,0x20000,word) && word==0xa5a5a5a5 && rom.calls==2);
 assert(!Q38NativePeerTestProgramWord(bytes,0,0x20000,word) && !Q38NativePeerTestProgramWord(bytes,120,0x20000,word)
  && !Q38NativePeerTestProgramWord(bytes,1,0x20001,word) && rom.calls==2);
 bytes.programWord=nullptr;assert(!Q38NativePeerTestProgramWord(bytes,1,0x20000,word) && word==0xa5a5a5a5 && rom.calls==2);
 bytes.programWord=Program;bytes.context=nullptr;assert(!Q38NativePeerTestProgramWord(bytes,1,0x20000,word) && rom.calls==2);
 puts("PASS immutable peer program DATA callback boundaries; NO worker execution or device");
}
