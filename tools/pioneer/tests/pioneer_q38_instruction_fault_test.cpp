/* SPDX-License-Identifier: MIT */
// Read-only diagnostic formatting only; no real owner/device is fabricated.
#include "PioneerQ38InstructionFault.h"
#include <array>
#include <cassert>
#include <map>
#include <string>
struct DiagnosticData {
 unsigned pc=0x60000,layer=15,completed=15,packets=1767309;
 std::array<unsigned,32> regs={};std::array<unsigned,10> command={};std::array<unsigned,8> counts={};
 std::map<unsigned,unsigned> memory;
};
int main(){
 namespace F=PioneerQ38InstructionFault;DiagnosticData data;
 data.regs[1]=0x50c38;data.regs[2]=0x6f340;data.regs[27]=0x6f700;
 for(unsigned base:{0x80000U,0x156000U,0x160000U,0xbc000U})for(unsigned i=0;i<5120;i++)data.memory[base+i*4]=0;
 data.memory[0x80000+3*4]=data.memory[0x156000+3*4]=0x477fe000;
 data.memory[0x80000+4*4]=1;data.memory[0x160000+5*4]=0x7fc00001;data.memory.erase(0xbc000+5119*4);
 const auto before=data;FILE* file=tmpfile();assert(file);
 F::Dump(data,0x50c34,0x80000,0x156000,0x160000,0xbc000,file);
 assert(data.memory==before.memory && data.regs==before.regs && data.command==before.command && data.counts==before.counts
  && data.pc==before.pc && data.layer==before.layer && data.completed==before.completed && data.packets==before.packets);
 assert(!fseek(file,0,SEEK_SET));std::string result;char buffer[4096];size_t bytes;
 while((bytes=fread(buffer,1,sizeof(buffer),file)))result.append(buffer,bytes);assert(!ferror(file));fclose(file);
 assert(result.find("previousPC00050c34 ra00050c38 sp0006f340 s110006f700")!=std::string::npos);
 assert(result.find("final address160000 missing0 nonfinite1")!=std::string::npos);
 assert(result.find("original addressbc000 missing1")!=std::string::npos);
 assert(result.find("invalidProjected1 invalidResidual0 invalidSum2")!=std::string::npos);
 assert(result.find("FAULT_FINAL_DOMAIN index3 a477fe000 b477fe000 sum")!=std::string::npos);
 for(unsigned b:{0U,0x80000000U,0x26800000U,0xa6800000U,0x477fe000U,0xc77fe000U})assert(F::Domain(b));
 for(unsigned b:{1U,0x80000001U,0x267fffffU,0x477fe001U,0x7f800000U,0x7fc00000U})assert(!F::Domain(b));
 puts("PASS read-only FAULT diagnostic/actual-PC+RA+SP+register/finite-range+sum/missing+NaN classification/12 domain edges/map+all fields UNCHANGED;UNIT DATA ONLY,no model/owner/device/launch authority");
}
