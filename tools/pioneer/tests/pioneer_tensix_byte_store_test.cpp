/* SPDX-License-Identifier: MIT */
// Offline ISA decoder test. No accelerator, model or device interaction.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
struct ByteStoreMachine:Machine {
 uint32_t storedAddress=0;uint8_t storedValue=0;unsigned stores=0;
 unsigned Index(uint32_t address)override{assert(address==0x6000);return 0;}
 bool CodeAddress(uint32_t address)const override{return address==0x6000;}
 void Store8(uint32_t address,uint8_t value)override{storedAddress=address;storedValue=value;stores++;}
};
int main(){
 ByteStoreMachine machine;
 for(int imm:{-2048,-1,0,1,2047}){
  const uint32_t bits=uint32_t(imm)&0xfffU;
  machine.pc=0x6000;machine.regs[1]=0x70000;machine.regs[2]=0xabcdef93;
  machine.l1[0]=((bits>>5)<<25)|(2U<<20)|(1U<<15)|((bits&31)<<7)|0x23U;
  const unsigned before=machine.stores;machine.Step();
  assert(machine.stores==before+1 && machine.storedAddress==uint32_t(0x70000+imm) && machine.storedValue==0x93);
 }
 puts("PASS offline RV32 byte-store decode and signed offsets; NO device/model/TPS");
}
